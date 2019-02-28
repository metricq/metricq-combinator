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

#include "calculation_node.hpp"
#include "log.hpp"

#include <metricq/json.hpp>
#include <metricq/types.hpp>

#include <fmt/format.h>

#include <string>
#include <vector>

class CombinedMetric
{
public:
    class ParseError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;

        template <typename Fmt, typename... Args>
        ParseError(Fmt&& format, Args&&... args)
        : std::runtime_error{ fmt::format(std::forward<Fmt>(format), std::forward<Args>(args)...) }
        {
        }
    };

public:
    CombinedMetric(const metricq::json&);
    CombinedMetric(CombinedMetric&&) = default;

    void update();

    InputNode& input()
    {
        return *input_;
    }

    MetricInputNodesByName collect_metric_inputs();

private:
    static std::unique_ptr<InputNode> parse_input(const metricq::json&);
    static std::unique_ptr<CalculationNode> parse_calc_node(const std::string& opstr,
                                                            std::unique_ptr<InputNode> left,
                                                            std::unique_ptr<InputNode> right);

private:
    std::unique_ptr<InputNode> input_;
};
