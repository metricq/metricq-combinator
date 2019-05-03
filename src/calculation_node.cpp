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
#include "calculation_node.hpp"

#include "log.hpp"

#include <metricq/types.hpp>

#include <fmt/format.h>

using Log = combinator_log::Log;

void MetricInputNode::collect_metric_inputs(MetricInputNodesByName& inputs)
{
    inputs[name_].emplace_back(this);
}

void BinaryCalculationNode::update()
{
    left_->update();
    right_->update();

    while (left_->has_input() && right_->has_input())
    {
        auto l = left_->peek();
        auto r = right_->peek();

        metricq::TimePoint new_time;
        {
            if (l.time < r.time)
            {
                new_time = l.time;
                left_->discard();
            }
            else if (l.time > r.time)
            {
                new_time = r.time;
                right_->discard();
            }
            else // (l.time == r.time)
            {
                new_time = l.time;
                left_->discard();
                right_->discard();
            }
        }

        auto new_value = combine(l.value, r.value);

        put(metricq::TimeValue{ new_time, new_value });
    }
    Log::trace() << fmt::format("Remaining queued values: {{ left: {}, right: {}, output: {} }}",
                                left_->queue_length(), right_->queue_length(), queue_length());
}

void BinaryCalculationNode::collect_metric_inputs(MetricInputNodesByName& inputs)
{
    left_->collect_metric_inputs(inputs);
    right_->collect_metric_inputs(inputs);
}

void VariadicCalculationNode::update()
{
    for (auto& input : input_nodes_)
    {
        input->update();
    }

    while (std::any_of(input_nodes_.begin(), input_nodes_.end(),
                       [](auto& input) { return input->has_input(); }))
    {
        std::vector<metricq::TimePoint> input_time_points;
        input_time_points.reserve(input_nodes_.size());
        std::vector<metricq::Value> input_values;
        input_values.reserve(input_nodes_.size());
        for (auto& input_node : input_nodes_)
        {
            auto tv = input_node->peek();
            input_time_points.emplace_back(tv.time);
            input_values.emplace_back(tv.value);
        }

        // Figure out the time all inputs agree on
        auto it = std::min_element(input_time_points.begin(), input_time_points.end());
        assert(it != input_time_points.end());
        auto new_time = *it;

        // discard all that are exactly up to the time
        // ZIP :(
        for (size_t i = 0; i < input_nodes_.size(); ++i)
        {
            auto& input = input_nodes_.at(i);
            auto& time = input_time_points.at(i);
            if (time == new_time)
            {
                input->discard();
            }
            else
            {
                assert(time > new_time);
            }
        }

        auto new_value = combine(input_values);
        put(metricq::TimeValue{ new_time, new_value });
    }
}

void VariadicCalculationNode::collect_metric_inputs(MetricInputNodesByName& inputs)
{
    for (auto& input_node : input_nodes_)
    {
        input_node->collect_metric_inputs(inputs);
    }
}
