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

#include <cmath>
#include <memory>
#include <utility>

struct BinaryNode : CalculationNode
{
public:
    BinaryNode(std::unique_ptr<InputNode> left, std::unique_ptr<InputNode> right)
    : left_(std::move(left)), right_(std::move(right))
    {
    }

    void update() override;
    virtual metricq::Value combine(metricq::Value a, metricq::Value b) = 0;

    void collect_metric_inputs(MetricInputNodesByName&) override;

private:
    std::unique_ptr<InputNode> left_;
    std::unique_ptr<InputNode> right_;
};

class AddNode : public BinaryNode
{
    metricq::Value combine(metricq::Value a, metricq::Value b) override
    {
        if (std::isnan(a))
        {
            return b;
        }
        if (std::isnan(b))
        {
            return a;
        }
        return a + b;
    }

public:
    using BinaryNode::BinaryNode;
};

class SubtractNode : public BinaryNode
{
    metricq::Value combine(metricq::Value a, metricq::Value b) override
    {
        return a - b;
    }

public:
    using BinaryNode::BinaryNode;
};

class MultipyNode : public BinaryNode
{
    metricq::Value combine(metricq::Value a, metricq::Value b) override
    {
        return a * b;
    }

public:
    using BinaryNode::BinaryNode;
};

class DivideNode : public BinaryNode
{
    metricq::Value combine(metricq::Value a, metricq::Value b) override
    {
        return a / b;
    }

public:
    using BinaryNode::BinaryNode;
};
