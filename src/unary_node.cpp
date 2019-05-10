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

#include "unary_node.hpp"

void UnaryNode::update()
{
    input_->update();

    while (input_->has_input())
    {
        auto tv = input_->peek();
        input_->discard();
        put(process(tv));
    }
}

void UnaryNode::collect_metric_inputs(MetricInputNodesByName& inputs)
{
    return input_->collect_metric_inputs(inputs);
}
