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

#include <cmath>

void VariadicNode::update()
{
    /*
     * For practical reasons, we give NaNs a special interpretation here:
     *
     * In particular, in practice, we often encounter the situation where a metric cannot be
     * read from the measurement device by the source, but the source itself is running fine. In
     * this situation, it is likely that the physical phenomenon can be assumed as non-existant.
     * For example, the power consumption of a PDU is zero, if the PDU failed. In such cases,
     * our source implementations will send NaNs as values.
     *
     * With this in mind, we treat NaNs in summations (and min()/max()) of metrics as zero, as
     * long as, there is at least a single metric with a non-NaN (including +-infinity) number.
     *
     * Note: This is not based on rigid mathematical definitions, but the reality of our setup.
     * Hence, NaNs aren't treated differently for the other operations.
     */

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
            if (!std::isnan(tv.value))
            {
                input_values.emplace_back(tv.value);
            }
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

        if (input_values.empty())
        {
            // All values are NaN
            put(metricq::TimeValue{ new_time, std::nan("") });
            return;
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
