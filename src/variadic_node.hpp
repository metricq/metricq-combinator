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
#include <utility>
#include <vector>

struct VariadicNode : CalculationNode
{
public:
    VariadicNode(std::vector<std::unique_ptr<InputNode>> inputs) : input_nodes_(std::move(inputs))
    {
    }

    void update() override;
    virtual metricq::Value combine(const std::vector<metricq::Value>& input_values) = 0;

    void collect_metric_inputs(MetricInputNodesByName&) override;

private:
    std::vector<std::unique_ptr<InputNode>> input_nodes_;
};

class SumNode : public VariadicNode
{
    metricq::Value combine(const std::vector<metricq::Value>& input_values) override
    {
        return std::accumulate(input_values.begin(), input_values.end(), metricq::Value());
    }

public:
    using VariadicNode::VariadicNode;
};

class MinNode : public VariadicNode
{
    metricq::Value combine(const std::vector<metricq::Value>& input_values) override
    {
        auto it = std::min_element(input_values.begin(), input_values.end());
        assert(it != input_values.end());
        return *it;
    }

public:
    using VariadicNode::VariadicNode;
};

class MaxNode : public VariadicNode
{
    metricq::Value combine(const std::vector<metricq::Value>& input_values) override
    {
        auto it = std::max_element(input_values.begin(), input_values.end());
        assert(it != input_values.end());
        return *it;
    }

public:
    using VariadicNode::VariadicNode;
};
