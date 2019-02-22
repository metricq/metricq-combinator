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

#include <deque>
#include <memory>
#include <optional>

struct NodeInput
{
    virtual bool has_input() const = 0;
    virtual metricq::TimeValue peek() const = 0;
    virtual void discard() = 0;

    virtual std::size_t queue_length() const = 0;
};

struct NodeOutput
{
    virtual void put(metricq::TimeValue value) = 0;

    virtual std::size_t queue_length() const = 0;
};

class NodeInOutQueue : public NodeInput, public NodeOutput
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

class ConstantInput : public NodeInput
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

class SinglyBufferedInput : public NodeInOutQueue
{
public:
    SinglyBufferedInput() : buffered_value_(Timestamp::genesis(), 0)
    {
    }

    bool has_input() const override
    {
        return NodeInOutQueue::has_input();
    }

    metricq::TimeValue peek() const override
    {
        return buffered_value_;
    }

    void discard() override
    {
        buffered_value_ = NodeInOutQueue::peek();
        NodeInOutQueue::discard();
    }

    std::size_t queue_length() const override
    {
        return 1 + NodeInOutQueue::queue_length();
    }

private:
    metricq::TimeValue buffered_value_;
};

struct CalculationNode
{
private:
    virtual metricq::Value combine(metricq::Value a, metricq::Value b) = 0;

public:
    void calculate(NodeInput& left, NodeInput& right, NodeOutput& output);
};

class AddNode : public CalculationNode
{
    metricq::Value combine(metricq::Value a, metricq::Value b) override
    {
        return a + b;
    }
};

class SubtractNode : public CalculationNode
{
    metricq::Value combine(metricq::Value a, metricq::Value b) override
    {
        return a - b;
    }
};

class MultipyNode : public CalculationNode
{
    metricq::Value combine(metricq::Value a, metricq::Value b) override
    {
        return a * b;
    }
};

class DivideNode : public CalculationNode
{
    metricq::Value combine(metricq::Value a, metricq::Value b) override
    {
        return a / b;
    }
};
