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

#include "timestamp.hpp"

#include <metricq/json.hpp>

#include <deque>
#include <memory>
#include <vector>

class MetricInputNode;
using MetricInputNodesByName = std::unordered_map<std::string, std::vector<MetricInputNode*>>;

struct InputNode
{
    virtual ~InputNode() = default;

    virtual bool has_input() const = 0;
    virtual metricq::TimeValue peek() const = 0;
    virtual void discard() = 0;

    virtual void update()
    {
    }

    virtual void collect_metric_inputs(MetricInputNodesByName&)
    {
    }

    virtual std::size_t queue_length() const = 0;
};

struct OutputNode
{
    virtual ~OutputNode() = default;

    virtual void put(metricq::TimeValue value) = 0;
    virtual std::size_t queue_length() const = 0;
};

class InputQueue : public InputNode, public OutputNode
{
public:
    void put(metricq::TimeValue tv) override
    {
        queue_.push_back(tv);
    }

    bool has_input() const override
    {
        return !queue_.empty();
    }

    metricq::TimeValue peek() const override
    {
        return queue_.front();
    }

    void discard() override
    {
        queue_.pop_front();
    }

    std::size_t queue_length() const override
    {
        return queue_.size();
    }

private:
    std::deque<metricq::TimeValue> queue_;
};

class ConstantInput : public InputNode
{
public:
    constexpr ConstantInput(metricq::Value value) : time_value_(Timestamp::armageddon(), value)
    {
    }

    bool has_input() const override
    {
        return true;
    }

    metricq::TimeValue peek() const override
    {
        return time_value_;
    }

    void discard() override
    {
    }

    metricq::TimeValue get_constant() const
    {
        return time_value_;
    }

    std::size_t queue_length() const override
    {
        return 1;
    }

private:
    metricq::TimeValue time_value_;
};

class MetricInputNode : public InputQueue
{
public:
    MetricInputNode(const std::string& name) : name_(name)
    {
    }

    void collect_metric_inputs(MetricInputNodesByName&) override;

    const std::string& name() const
    {
        return name_;
    }

private:
    std::string name_;
};

class SinglyBufferedInputQueue : public InputQueue
{
public:
    SinglyBufferedInputQueue() : buffered_value_(Timestamp::genesis(), 0)
    {
    }

    bool has_input() const override
    {
        return InputQueue::has_input();
    }

    metricq::TimeValue peek() const override
    {
        return buffered_value_;
    }

    void discard() override
    {
        buffered_value_ = InputQueue::peek();
        InputQueue::discard();
    }

    std::size_t queue_length() const override
    {
        return 1 + InputQueue::queue_length();
    }

private:
    metricq::TimeValue buffered_value_;
};

struct CalculationNode : InputQueue
{
};

struct BinaryCalculationNode : CalculationNode
{
public:
    BinaryCalculationNode(std::unique_ptr<InputNode> left, std::unique_ptr<InputNode> right)
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

class AddNode : public BinaryCalculationNode
{
    metricq::Value combine(metricq::Value a, metricq::Value b) override
    {
        return a + b;
    }

public:
    using BinaryCalculationNode::BinaryCalculationNode;
};

class SubtractNode : public BinaryCalculationNode
{
    metricq::Value combine(metricq::Value a, metricq::Value b) override
    {
        return a - b;
    }

public:
    using BinaryCalculationNode::BinaryCalculationNode;
};

class MultipyNode : public BinaryCalculationNode
{
    metricq::Value combine(metricq::Value a, metricq::Value b) override
    {
        return a * b;
    }

public:
    using BinaryCalculationNode::BinaryCalculationNode;
};

class DivideNode : public BinaryCalculationNode
{
    metricq::Value combine(metricq::Value a, metricq::Value b) override
    {
        return a / b;
    }

public:
    using BinaryCalculationNode::BinaryCalculationNode;
};

struct VariadicCalculationNode : CalculationNode
{
public:
    VariadicCalculationNode(std::vector<std::unique_ptr<InputNode>> inputs)
    : input_nodes_(std::move(inputs))
    {
    }

    void update() override;
    virtual metricq::Value combine(const std::vector<metricq::Value>& input_values) = 0;

    void collect_metric_inputs(MetricInputNodesByName&) override;

private:
    std::vector<std::unique_ptr<InputNode>> input_nodes_;
};

class SumNode : public VariadicCalculationNode
{
    metricq::Value combine(const std::vector<metricq::Value>& input_values) override
    {
        return std::accumulate(input_values.begin(), input_values.end(), metricq::Value());
    }

public:
    using VariadicCalculationNode::VariadicCalculationNode;
};

class MinNode : public VariadicCalculationNode
{
    metricq::Value combine(const std::vector<metricq::Value>& input_values) override
    {
        auto it = std::min_element(input_values.begin(), input_values.end());
        assert(it != input_values.end());
        return *it;
    }

public:
    using VariadicCalculationNode::VariadicCalculationNode;
};

class MaxNode : public VariadicCalculationNode
{
    metricq::Value combine(const std::vector<metricq::Value>& input_values) override
    {
        auto it = std::max_element(input_values.begin(), input_values.end());
        assert(it != input_values.end());
        return *it;
    }

public:
    using VariadicCalculationNode::VariadicCalculationNode;
};
