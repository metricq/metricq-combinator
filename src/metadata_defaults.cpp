// metricq-combinator
// Copyright (C) 2026 ZIH, Technische Universitaet Dresden, Federal Republic of Germany
//
// All rights reserved.
//
// This file is part of metricq-combinator.
//
// metricq-combinator is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// metricq-combinator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with metricq-combinator.  If not, see <http://www.gnu.org/licenses/>.
#include "metadata_defaults.hpp"

#include <metricq/json.hpp>

namespace combinator
{
const std::unordered_set<std::string> reserved_metadata_keys = { "rate", "chunkSize", "source",
                                                                 "date", "historic",  "interval" };

void apply_metadata_defaults(metricq::Metadata& target,
                             const std::vector<const metricq::Metadata*>& inputs,
                             const std::unordered_set<std::string>& excluded_keys)
{
    if (inputs.empty())
    {
        return;
    }

    // Candidate keys: the union of all keys present in any input, minus excluded keys, keys
    // already set on the target, and keys starting with "_" (reserved by the MetricQ
    // metadata backend itself, e.g. "_id" -- this is not specific to any caller's
    // excluded_keys, so it is enforced here unconditionally).
    std::unordered_set<std::string> candidate_keys;
    for (const auto* input : inputs)
    {
        for (const auto& [key, value] : input->json().items())
        {
            if (!key.empty() && key.front() == '_')
            {
                continue;
            }
            if (excluded_keys.count(key) == 0 && target.json().count(key) == 0)
            {
                candidate_keys.insert(key);
            }
        }
    }

    for (const auto& key : candidate_keys)
    {
        // A key is only defaulted if every single input sets it, and all of them agree on
        // its value. A key that's simply absent from some inputs (e.g. a ConstantInput
        // with no metadata at all) is not "agreement" and must not be defaulted either.
        const metricq::json* agreed_value = nullptr;
        bool all_inputs_agree = true;

        for (const auto* input : inputs)
        {
            auto it = input->json().find(key);
            if (it == input->json().end())
            {
                all_inputs_agree = false;
                break;
            }

            if (agreed_value == nullptr)
            {
                agreed_value = &(*it);
            }
            else if (*agreed_value != *it)
            {
                all_inputs_agree = false;
                break;
            }
        }

        if (all_inputs_agree)
        {
            target[key] = *agreed_value;
        }
    }
}
} // namespace combinator
