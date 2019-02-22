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

#include <iosfwd>
#include <string>
#include <variant>
#include <vector>

using json = metricq::json;

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
    class MetricInput : public NodeInOutQueue
    {
    public:
        MetricInput(const std::string& name) : name_(name)
        {
        }

        const std::string& name() const
        {
            return name_;
        }

    private:
        std::string name_;
    };

public:
    CombinedMetric(const json&);
    CombinedMetric(CombinedMetric&&) = default;

    NodeInput& as_input()
    {
        return result_queue_;
    }

    NodeOutput& as_output()
    {
        return result_queue_;
    }

    void calculate();

    friend std::ostream& operator<<(std::ostream&, const CombinedMetric&);

    using MetricInputsByName = std::unordered_map<std::string, std::vector<MetricInput*>>;

    MetricInputsByName collect_metric_inputs();

private:
    void collect_metric_inputs(MetricInputsByName& inputs);

    using Input = std::variant<MetricInput, ConstantInput, std::unique_ptr<CombinedMetric>>;

    static Input parse_input(const json&);
    static std::unique_ptr<CalculationNode> parse_calc_node(const std::string&);

    static NodeInput& get_updated_input(Input& input);

private:
    Input left_;
    Input right_;
    std::unique_ptr<CalculationNode> node_;
    NodeInOutQueue result_queue_;
};

std::ostream& operator<<(std::ostream&, const CombinedMetric&);
