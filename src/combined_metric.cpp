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

#include <metricq/json.hpp>

std::unique_ptr<CalculationNode> CombinedMetric::parse_calc_node(const std::string& opstr,
                                                                 std::unique_ptr<InputNode> left,
                                                                 std::unique_ptr<InputNode> right)
{
    if (opstr.length() != 1)
    {
        throw CombinedMetric::ParseError("unknown operation \"{}\"", opstr);
    }

    if (opstr == "+")
    {
        return std::make_unique<AddNode>(std::move(left), std::move(right));
    }
    if (opstr == "-")
    {
        return std::make_unique<SubtractNode>(std::move(left), std::move(right));
    }
    if (opstr == "*")
    {
        return std::make_unique<MultipyNode>(std::move(left), std::move(right));
    }
    if (opstr == "/")
    {
        return std::make_unique<DivideNode>(std::move(left), std::move(right));
    }
    if (opstr == "min")
    {
        return std::make_unique<MinNode>(std::move(left), std::move(right));
    }
    if (opstr == "max")
    {
        return std::make_unique<MaxNode>(std::move(left), std::move(right));
    }
    throw CombinedMetric::ParseError("unknown operation \"{}\"", opstr);
}

std::unique_ptr<InputNode> CombinedMetric::parse_input(const metricq::json& config)
{
    try
    {
        if (config.is_number())
        {
            return std::make_unique<ConstantInput>(config.get<double>());
        }
        else if (config.is_string())
        {
            return std::make_unique<MetricInputNode>(config.get<std::string>());
        }
        else if (config.is_object())
        {
            // TODO: Check that not both of left and right are ConstantInput.
            // Otherwise the update algorithm will constantly try to update the
            // CombinedMetric, since both of its inputs have values ready.
            std::unique_ptr<InputNode> left = parse_input(config.at("left"));
            std::unique_ptr<InputNode> right = parse_input(config.at("right"));

            return parse_calc_node(config.at("operation"), std::move(left), std::move(right));
        }
        else
        {
            throw ParseError("invalid configuration for metric input: ", config.dump());
        }
    }
    catch (const metricq::json::exception& e)
    {
        throw ParseError("failed to parse configuration: {}", e.what());
    }
}

CombinedMetric::CombinedMetric(const metricq::json& config) : input_(parse_input(config))
{
}

void CombinedMetric::update()
{
    input_->update();
}

MetricInputNodesByName CombinedMetric::collect_metric_inputs()
{
    MetricInputNodesByName inputs;
    input_->collect_metric_inputs(inputs);
    return inputs;
}
