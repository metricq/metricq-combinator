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
#include "combined_metric.hpp"

#include <nlohmann/json.hpp>

#include <cassert>
#include <ostream>

std::unique_ptr<CalculationNode> CombinedMetric::parse_calc_node(const std::string& opstr)
{
    if (opstr.length() != 1)
    {
        throw CombinedMetric::ParseError("unknown operation '{}'", opstr);
    }

    switch (opstr[0])
    {
    case '+':
        return std::make_unique<AddNode>();
        break;
    case '-':
        return std::make_unique<SubtractNode>();
        break;
    case '*':
        return std::make_unique<MultipyNode>();
        break;
    case '/':
        return std::make_unique<DivideNode>();
        break;
    default:
        throw CombinedMetric::ParseError("unknown operation \"{}\"", opstr);
    }
}

CombinedMetric::Input CombinedMetric::parse_input(const json& config)
{
    try
    {
        if (config.is_number())
        {
            return { ConstantInput(config.get<double>()) };
        }
        else if (config.is_string())
        {
            return { MetricInput(config.get<std::string>()) };
        }
        else if (config.is_object())
        {
            return { std::make_unique<CombinedMetric>(config) };
        }
        else
        {
            throw ParseError("invalid configuration for metric input: ", config.dump());
        }
    }
    catch (const json::exception& e)
    {
        throw ParseError("failed to parse configuration: {}", e.what());
    }
}

// TODO: Check that not both of left_ and right_ are ConstantInput.
// Otherwise the update algorithm will constantly try to update the
// CombinedMetric, since both of its inputs have values ready.
CombinedMetric::CombinedMetric(const json& config)
: left_(parse_input(config.at("left"))), right_(parse_input(config.at("right"))),
  node_(parse_calc_node(config.at("operation").get<std::string>()))
{
}

NodeInput& CombinedMetric::get_updated_input(Input& input)
{
    struct get_inputs
    {
        NodeInput& operator()(MetricInput& metric)
        {
            return metric;
        }

        NodeInput& operator()(ConstantInput& constant)
        {
            return constant;
        }

        NodeInput& operator()(const std::unique_ptr<CombinedMetric>& combined)
        {
            combined->calculate();
            return combined->as_input();
        }
    };

    return std::visit(get_inputs{}, input);
}

void CombinedMetric::calculate()
{
    node_->calculate(get_updated_input(left_), get_updated_input(right_), result_queue_);
}

CombinedMetric::MetricInputsByName CombinedMetric::collect_metric_inputs()
{
    MetricInputsByName inputs;
    collect_metric_inputs(inputs);
    return inputs;
}

void CombinedMetric::collect_metric_inputs(MetricInputsByName& metric_input)
{
    struct metric_input_collector
    {
        void operator()(MetricInput& metric)
        {
            collected_inputs[metric.name()].push_back(&metric);
        }

        void operator()(ConstantInput&)
        {
        }

        void operator()(const std::unique_ptr<CombinedMetric>& combined)
        {
            combined->collect_metric_inputs(collected_inputs);
        }

        MetricInputsByName& collected_inputs;
    };

    std::visit(metric_input_collector{ metric_input }, left_);
    std::visit(metric_input_collector{ metric_input }, right_);
}

std::ostream& operator<<(std::ostream& os, const CombinedMetric& metric)
{
    struct print
    {
        std::ostream& operator()(const CombinedMetric::MetricInput& metric)
        {
            return os << metric.name();
        }

        std::ostream& operator()(const ConstantInput& constant)
        {
            return os << constant.get_constant().value;
        }

        std::ostream& operator()(const std::unique_ptr<CombinedMetric>& combined)
        {
            return os << *combined;
        }

        std::ostream& os;
    };

    os << "(";
    std::visit(print{ os }, metric.left_);
    os << " ??? ";
    std::visit(print{ os }, metric.right_);
    os << ")";
    return os;
}
