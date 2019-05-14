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
#pragma once

#include "input_node.hpp"

#include <metricq/types.hpp>

#include <memory>

struct ThrottleNode : CalculationNode
{
public:
    ThrottleNode(std::unique_ptr<InputNode> input, metricq::Duration cooldown_period)
    : input_(std::move(input)), cooldown_period_(cooldown_period)
    {
    }

    void update() override;

    void collect_metric_inputs(MetricInputNodesByName&) override;

private:
    std::unique_ptr<InputNode> input_;
    metricq::Duration cooldown_period_;
    metricq::TimePoint last_time_point_ = Timestamp::genesis();
};