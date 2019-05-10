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

#include "variadic_node.hpp"

void VariadicNode::update()
{
    for (auto& input : input_nodes_)
    {
        input->update();
    }

    while (std::all_of(input_nodes_.begin(), input_nodes_.end(),
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
            auto& time = input_time_points.at(i);
            if (time == new_time)
            {
                auto& input = input_nodes_.at(i);
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

void VariadicNode::collect_metric_inputs(MetricInputNodesByName& inputs)
{
    for (auto& input_node : input_nodes_)
    {
        input_node->collect_metric_inputs(inputs);
    }
}
