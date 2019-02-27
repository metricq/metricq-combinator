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

void CalculationNode::update()
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

void MetricInputNode::collect_metric_inputs(MetricInputNodesByName& inputs)
{
    inputs[name_].emplace_back(this);
}

void CalculationNode::collect_metric_inputs(MetricInputNodesByName& inputs)
{
    left_->collect_metric_inputs(inputs);
    right_->collect_metric_inputs(inputs);
}