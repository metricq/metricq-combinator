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

std::unique_ptr<CalculationNode> CombinedMetric::parse_calc_node(const metricq::json& config)
{
    // TODO: Check that not all inputs are ConstantInput.
    // Otherwise the update algorithm will constantly try to update the
    // CombinedMetric, since both of its inputs have values ready.

    std::string op = config.at("operation");
    if (op == "+")
    {
        return std::make_unique<AddNode>(parse_input(config.at("left")),
                                         parse_input(config.at("right")));
    }
    if (op == "-")
    {
        return std::make_unique<SubtractNode>(parse_input(config.at("left")),
                                              parse_input(config.at("right")));
    }
    if (op == "*")
    {
        return std::make_unique<MultipyNode>(parse_input(config.at("left")),
                                             parse_input(config.at("right")));
    }
    if (op == "/")
    {
        return std::make_unique<DivideNode>(parse_input(config.at("left")),
                                            parse_input(config.at("right")));
    }
    if (op == "min")
    {
        return std::make_unique<MinNode>(parse_inputs(config.at("inputs")));
    }
    if (op == "max")
    {
        return std::make_unique<MaxNode>(parse_inputs(config.at("inputs")));
    }
    if (op == "sum")
    {
        return std::make_unique<SumNode>(parse_inputs(config.at("inputs")));
    }
    throw CombinedMetric::ParseError("unknown operation \"{}\"", op);
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
            return parse_calc_node(config);
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

std::vector<std::unique_ptr<InputNode>> CombinedMetric::parse_inputs(const metricq::json& configs)
{
    if (!configs.is_array())
    {
        throw ParseError("inputs are not an array: ", configs.dump());
    }
    if (!configs.size() == 0)
    {
        throw ParseError("empty inputs array.");
    }
    std::vector<std::unique_ptr<InputNode>> result;
    result.reserve(configs.size());
    for (const auto& config : configs)
    {
        result.emplace_back(parse_input(config));
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
