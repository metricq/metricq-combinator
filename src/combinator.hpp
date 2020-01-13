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

#include "combined_metric.hpp"
#include "input_node.hpp"

#include <asio/signal_set.hpp>
#include <metricq/transformer.hpp>

class Combinator : public metricq::Transformer
{
private:
    using MetricName = std::string;

public:
    Combinator(const std::string& manager_host, const std::string& token);
    ~Combinator();

private:
    void on_transformer_config(const metricq::json& config) override;
    void on_transformer_ready() override;
    void on_data(const std::string& metric_name, const metricq::DataChunk&) override;

    metricq::Metric<metricq::Transformer>& get_combined_metric(const std::string& combined_name)
    {
        return (*this)[combined_name];
    }

private:
    struct CombinedMetricContainer
    {
    private:
        CombinedMetricContainer(const metricq::json& config)
        : metric(config), inputs(metric.collect_metric_inputs()), expression_(config)
        {
        }

    public:
        static CombinedMetricContainer from_config(const metricq::json& config)
        {
            return CombinedMetricContainer(config);
        }

        const metricq::json& expression() const
        {
            return expression_;
        }

        CombinedMetric metric;
        MetricInputNodesByName inputs;
        metricq::json expression_;
    };

    using CombinedMetricByName = std::unordered_map<MetricName, CombinedMetricContainer>;

    asio::signal_set signals_;
    CombinedMetricByName combined_metrics_;
};
