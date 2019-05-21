// metricq-combinator
// Copyright (C) 2019 ZIH, Technische Universitaet Dresden, Federal Republic of Germany
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
#include "combinator.hpp"

#include <metricq/logger/nitro.hpp>
#include <metricq/source.hpp>

#include <fmt/format.h>

#include <numeric>

using Log = metricq::logger::nitro::Log;

Combinator::Combinator(const std::string& manager_host, const std::string& token)
: metricq::Transformer(token), signals_(io_service, SIGINT, SIGTERM)
{
    signals_.async_wait([this](auto, auto signal) {
        if (!signal)
        {
            return;
        }

        Log::info() << "Shutting down... (received signal " << signal << ")";
        close();
    });

    connect(manager_host);
}

Combinator::~Combinator()
{
}

void Combinator::on_transformer_config(const metricq::json& config)
{
    Log::trace() << "config: " << config;
    auto& combined_metrics = config.at("metrics");
    for (auto it = combined_metrics.begin(); it != combined_metrics.end(); ++it)
    {
        auto& combined_config = it.value();
        auto combined_name = it.key();

        auto [container_it, success] = combined_metrics_.emplace(
            combined_name, CombinedMetricContainer::from_config(combined_config.at("expression")));

        if (!success)
        {
            Log::error() << "Multiple definitions of combined metric '" << combined_name
                         << "', ignoring duplicates";
            continue;
        }

        auto& combined_metric = container_it->second.metric;

        Log::info() << "Deriving new metric: " << combined_name;

        // Register input metrics with sink
        for (const auto& [input_name, _] : combined_metric.collect_metric_inputs())
        {
            input_metrics.emplace_back(input_name);
        }

        // Register the combined metric as a new source metric
        auto& metric = (*this)[combined_name];

        if (combined_config.count("chunk_size"))
        {
            auto chunk_size = combined_config["chunk_size"].get<int>();
            if (chunk_size > 0)
            {
                metric.chunk_size(chunk_size);
                Log::debug() << fmt::format("Using chunk_size ({}) for metric '{}'.", chunk_size,
                                            combined_name);
            }
            else
            {
                Log::warn() << fmt::format(
                    "The given chunk_size ({}) for the metric '{}' is invalid. Ignoring.",
                    chunk_size, combined_name);
            }
        }

        // Optionally declare metadata for this combined metric, which are
        // sourced from combined_config["metadata"], if the key exists
        if (auto metadata_it = combined_config.find("metadata");
            metadata_it != combined_config.end())
        {
            auto& metadata = *metadata_it;
            if (metadata.is_object())
            {
                Log::debug() << "Declaring metadata for metric " << combined_name << ": "
                             << metadata.dump();
                metric.metadata.json(metadata);
            }
            else
            {
                Log::warn() << "Metadata for combined metric " << combined_name
                            << " are not an object, ignoring.";
            }
        }
    }
}

void Combinator::on_transformer_ready()
{
    // At this point, the metadata of all direct input metrics is available in this->metadata_
    // so we can calculate the rate of the combined metrics.
    // However, the metedata of input metrics that are combined from the same config may not yet be
    // available, so we need to be able to defer these metrics.
    bool missing_inputs = false;

    std::queue<const CombinedMetricByName::value_type*> resolver_queue;

    for (const auto& elem : combined_metrics_)
    {
        resolver_queue.emplace(&elem);
    }

    std::size_t max_deferrals = (resolver_queue.size() - 2) * (resolver_queue.size() - 1) / 2;
    while (!resolver_queue.empty())
    {
        auto current_queue_element = resolver_queue.front();
        auto& [combined_name, metric_container] = *current_queue_element;
        resolver_queue.pop();

        auto& metric = get_combined_metric(combined_name);

        // do not overwrite if rate was already set in the config
        if (std::isnan(metric.metadata.rate()))
        {
            auto rate = 0.;

            for (auto& [input_metric, input_nodes] : metric_container.inputs)
            {
                // if rate was not set, this returns NaN, which will propagate through
                try
                {
                    rate = std::max(rate, metadata_.at(input_metric).rate());
                }
                catch (const std::out_of_range&)
                {
                    if (combined_metrics_.count(input_metric))
                    {
                        Log::info() << "deferring resolving of indirectly combined metric "
                                    << combined_name << " due to yet missing " << input_metric;
                        resolver_queue.emplace(current_queue_element);
                        if (max_deferrals == 0)
                        {
                            Log::fatal() << "Maximum deferral count exceeded. Is there a circular "
                                            "dependency in your combinator config?";
                            throw std::runtime_error("could not resolve metric dependencies");
                        }
                        max_deferrals--;
                        // It was PHILIPP!!!
                        goto continue_main_loop;
                    }

                    Log::error() << "Missing input " << input_metric << " for combined "
                                 << combined_name;
                    missing_inputs = true;
                }
            }

            if (!std::isnan(rate))
            {
                metric.metadata.rate(rate);
            }
        }
        metadata_[combined_name] = metric.metadata;
        // It was PHILIPP!!!
    continue_main_loop:;
    }

    if (missing_inputs)
    {
        Log::fatal() << "Aborting due to missing input(s)";
        throw std::runtime_error("missing inputs");
    }

    Log::info() << "Combinator ready.";
}

void Combinator::on_data(const std::string& input_metric, const metricq::DataChunk& data)
{
    Log::trace() << fmt::format("Got data from input metric {}", input_metric);

    for (auto& [combined_name, metric_container] : combined_metrics_)
    {
        auto& [combined_metric, inputs_by_name] = metric_container;

        Log::trace() << fmt::format("Checking whether combined metric {} depends on {}...",
                                    combined_name, input_metric);

        auto inputs = inputs_by_name.find(input_metric);
        if (inputs == inputs_by_name.end())
        {
            Log::trace() << "└── No, it does not.";
            continue;
        }

        Log::trace() << "└── Yes, it does.";
        auto& input_list = inputs->second;
        for (MetricInputNode* input_node : input_list)
        {
            for (metricq::TimeValue tv : data)
            {
                input_node->put(tv);
            }
            Log::trace() << fmt::format("└── Put data into queue ({:p}), now has length {}",
                                        (void*)input_node, input_node->queue_length());
        }

        combined_metric.update();

        auto& metricq_metric = get_combined_metric(combined_name);
        InputNode& input = combined_metric.input();
        while (input.has_input())
        {
            metricq_metric.send(input.peek());
            input.discard();
        }
    }
}
