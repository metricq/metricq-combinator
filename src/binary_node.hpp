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
