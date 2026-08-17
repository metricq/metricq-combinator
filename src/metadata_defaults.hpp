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
#pragma once

#include <metricq/metadata.hpp>

#include <string>
#include <unordered_set>
#include <vector>

namespace combinator
{
// Metadata keys that MetricQ treats as technical or reserved for a metric (see
// https://metricq.github.io/metricq-python/metadata.html) and that a combined metric must
// therefore never inherit as a default value from its inputs:
//  - "rate": aggregated separately (max over inputs) by the combinator.
//  - "chunkSize": a property of this metric's own output, derived from
//    combined_config["chunk_size"].
//  - "source": the producing MetricQ source, which for a combined metric is the combinator
//    itself, never whatever produced its input(s).
//  - "date": timestamp of the last metadata update, which belongs to this new combined
//    metric, not to when its inputs were last updated.
//  - "historic": set by the manager, describes whether this specific metric is stored
//    historically.
//  - "interval": the reciprocal of "rate", which is computed separately; an inherited
//    "interval" could contradict it.
// (Keys starting with "_", e.g. "_id", are reserved for the metadata backend and are always
// excluded by apply_metadata_defaults() itself, regardless of this list.)
//
// This is shared between the production code (Combinator::on_transformer_ready()) and its
// tests, so that a test verifying the exclusion behaves correctly can't pass while the
// production call site silently stops excluding one of these keys.
extern const std::unordered_set<std::string> reserved_metadata_keys;

// Fills in default values on `target` for every metadata key that is set, with the same
// value, on *every* one of `inputs` -- unless the key is already set on `target` (e.g. from
// explicit config), listed in `excluded_keys` (fields with their own dedicated handling,
// such as "rate" and "chunkSize"), or starts with "_" (reserved by the MetricQ metadata
// backend, e.g. "_id" -- always excluded, regardless of `excluded_keys`). A key that is
// missing from even a single input, or that inputs disagree on, is left unset rather than
// guessing.
void apply_metadata_defaults(metricq::Metadata& target,
                             const std::vector<const metricq::Metadata*>& inputs,
                             const std::unordered_set<std::string>& excluded_keys);
} // namespace combinator
