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
#include <unordered_map>
#include <vector>

namespace combinator
{
struct MetadataResolution
{
    // Final metadata (explicit config, plus derived defaults and aggregated rate) for every
    // combined metric named in `combined_metric_inputs`.
    std::unordered_map<std::string, metricq::Metadata> metadata;
    // Set if at least one combined metric referenced an input metric that is neither one of
    // the other combined metrics (in which case resolving it would just be deferred) nor
    // present in `external_metadata`.
    bool missing_inputs = false;
};

// Resolves the final metadata for a whole generation of combined metrics, mirroring
// Combinator::on_transformer_ready(): combined metrics may depend on each other (indirectly
// combined inputs), so entries whose dependencies aren't resolved yet are deferred and
// retried once they are. Throws std::runtime_error if dependencies can't be resolved within
// a bounded number of retries (a strong indicator of a circular dependency in the config).
//
//  - combined_metric_inputs: for each combined metric name, the (possibly indirect) metric
//    names it depends on, as produced by CombinedMetric::collect_metric_inputs().
//  - explicit_metadata: each combined metric's metadata as set explicitly via config (see
//    Combinator::on_transformer_config()); combined metrics not present here start from
//    empty metadata.
//  - external_metadata: metadata of the externally subscribed input metrics, as provided by
//    the MetricQ manager.
MetadataResolution resolve_combined_metadata(
    const std::unordered_map<std::string, std::vector<std::string>>& combined_metric_inputs,
    const std::unordered_map<std::string, metricq::Metadata>& explicit_metadata,
    const std::unordered_map<std::string, metricq::Metadata>& external_metadata);
} // namespace combinator
