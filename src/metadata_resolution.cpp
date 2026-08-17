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
#include "metadata_resolution.hpp"
#include "metadata_defaults.hpp"

#include <metricq/logger/nitro.hpp>

#include <cmath>
#include <queue>
#include <stdexcept>

namespace combinator
{
namespace
{
    using Log = metricq::logger::nitro::Log;
}

MetadataResolution resolve_combined_metadata(
    const std::unordered_map<std::string, std::vector<std::string>>& combined_metric_inputs,
    const std::unordered_map<std::string, metricq::Metadata>& explicit_metadata,
    const std::unordered_map<std::string, metricq::Metadata>& external_metadata)
{
    MetadataResolution result;

    for (const auto& [combined_name, inputs] : combined_metric_inputs)
    {
        (void)inputs;
        if (auto it = explicit_metadata.find(combined_name); it != explicit_metadata.end())
        {
            result.metadata.emplace(combined_name, it->second);
        }
        else
        {
            result.metadata.emplace(combined_name, metricq::Metadata{});
        }
    }

    // Resolved metadata for lookup while resolving: external input metrics up front, plus
    // combined metrics as they get resolved below (so combined metrics may depend on each
    // other). Pointers into result.metadata stay valid for the lifetime of this function:
    // all of its entries were already inserted above, so nothing here triggers a rehash.
    std::unordered_map<std::string, const metricq::Metadata*> resolved_metadata;
    for (const auto& [name, metadata] : external_metadata)
    {
        resolved_metadata.emplace(name, &metadata);
    }

    std::queue<const std::string*> resolver_queue;
    for (const auto& [combined_name, inputs] : combined_metric_inputs)
    {
        (void)inputs;
        resolver_queue.push(&combined_name);
    }

    // Worst-case number of deferrals needed to resolve a dependency chain of n combined
    // metrics, should the queue's initial (hash-map-driven, effectively arbitrary)
    // processing order happen to be the exact reverse of the dependency order: the metric at
    // chain depth d (counting from the one with no combined-metric dependency) may need to
    // be deferred up to d times, for a worst-case total of 1 + 2 + ... + (n - 1) = n*(n-1)/2.
    const std::size_t n = resolver_queue.size();
    std::size_t max_deferrals = n * (n - 1) / 2;
    while (!resolver_queue.empty())
    {
        const std::string& combined_name = *resolver_queue.front();
        resolver_queue.pop();

        auto& metric_metadata = result.metadata.at(combined_name);

        // do not overwrite rate if it was already set in the config
        bool compute_rate = std::isnan(metric_metadata.rate());
        auto rate = 0.;

        std::vector<const metricq::Metadata*> input_metadatas;
        bool deferred = false;

        for (const auto& input_metric : combined_metric_inputs.at(combined_name))
        {
            auto it = resolved_metadata.find(input_metric);
            if (it == resolved_metadata.end())
            {
                if (combined_metric_inputs.count(input_metric))
                {
                    Log::info() << "deferring resolving of indirectly combined metric "
                                << combined_name << " due to yet missing " << input_metric;
                    resolver_queue.push(&combined_name);
                    if (max_deferrals == 0)
                    {
                        Log::fatal() << "Maximum deferral count exceeded. Is there a circular "
                                        "dependency in your combinator config?";
                        throw std::runtime_error("could not resolve metric dependencies");
                    }
                    max_deferrals--;
                    deferred = true;
                    break;
                }

                Log::error() << "Missing input " << input_metric << " for combined "
                             << combined_name;
                result.missing_inputs = true;
                continue;
            }

            input_metadatas.push_back(it->second);

            if (compute_rate)
            {
                // if rate was not set, this returns NaN, which will propagate through
                rate = std::max(rate, it->second->rate());
            }
        }

        if (deferred)
        {
            continue;
        }

        apply_metadata_defaults(metric_metadata, input_metadatas, reserved_metadata_keys);

        if (compute_rate && !std::isnan(rate))
        {
            metric_metadata.rate(rate);
        }

        resolved_metadata[combined_name] = &metric_metadata;
    }

    return result;
}
} // namespace combinator
