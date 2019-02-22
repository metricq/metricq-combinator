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

#include <fmt/format.h>

using Log = combinator_log::Log;

void CalculationNode::calculate(NodeInput& left, NodeInput& right, NodeOutput& output)
{
    while (left.has_input() && right.has_input())
    {
        auto l = left.peek();
        auto r = right.peek();

        metricq::TimePoint new_time;
        {
            if (l.time < r.time)
            {
                new_time = l.time;
                left.discard();
            }
            else if (l.time > r.time)
            {
                new_time = r.time;
                right.discard();
            }
            else // (l.time == r.time)
            {
                new_time = l.time;
                left.discard();
                right.discard();
            }
        }

        auto new_value = combine(l.value, r.value);

        output.put(metricq::TimeValue{ new_time, new_value });
    }
    Log::trace() << fmt::format("Remaining queued values: {{ left: {}, right: {}, output: {} }}",
                                left.queue_length(), right.queue_length(), output.queue_length());
}
