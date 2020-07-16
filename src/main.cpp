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
#include "combinator.hpp"

#include <metricq/exception.hpp>
#include <metricq/logger/nitro.hpp>
#include <metricq/version.hpp>

#include <nitro/broken_options/parser.hpp>

#include <cstdlib>

using Log = metricq::logger::nitro::Log;

// that's in <cstdlib>
// constexpr int EXIT_FAILURE = 1;
constexpr int EXIT_RESTART = 2;

nitro::broken_options::options parse_options(int argc, const char* argv[])
{
    metricq::logger::nitro::set_severity(nitro::log::severity_level::debug);

    nitro::broken_options::parser parser;
    parser.option("server", "The metricq management server to connect to.")
        .default_value("amqp://localhost")
        .short_name("s");
    parser
        .option("token",
                "The token used for transformer authentication against the metricq manager.")
        .default_value("combinator-dummy");
    parser.toggle("verbose").short_name("v");
    parser.toggle("trace").short_name("t");
    parser.toggle("quiet").short_name("q");
    parser.toggle("help").short_name("h");

    std::string config_filename;

    try
    {
        auto options = parser.parse(argc, argv);

        metricq::logger::nitro::initialize();

        if (options.given("help"))
        {
            parser.usage();
            std::exit(EXIT_SUCCESS);
        }

        if (options.given("trace"))
        {
            metricq::logger::nitro::set_severity(nitro::log::severity_level::trace);
        }
        else if (options.given("verbose"))
        {
            metricq::logger::nitro::set_severity(nitro::log::severity_level::debug);
        }
        else if (options.given("quiet"))
        {
            metricq::logger::nitro::set_severity(nitro::log::severity_level::warn);
        }

        return options;
    }
    catch (nitro::broken_options::parsing_error& e)
    {
        Log::warn() << "Error parsing options: " << e.what();
        parser.usage();
        std::exit(EXIT_FAILURE); // 1
    }
    catch (std::exception& e)
    {
        Log::error() << "Unhandled exception: " << e.what();
        parser.usage();
        std::exit(EXIT_FAILURE); // 1
    }
}

int main(int argc, const char* argv[])
{
    auto options = parse_options(argc, argv);

    try
    {
        Combinator combinator{
            options.get("server"),
            options.get("token"),
        };

        Log::info() << "MetricQ version " << metricq::version();
        Log::info() << "starting main loop...";
        combinator.main_loop();
        Log::info() << "stopped.";
    }
    catch (const CombinedMetric::ParseError& e)
    {
        Log::error() << "Error parsing options: " << e.what();
        return EXIT_FAILURE;
    }
    catch (const metricq::RPCError& e)
    {
        Log::error() << "RPC error: " << e.what();
        return EXIT_FAILURE;
    }
    catch (const std::exception& e)
    {
        Log::error() << "Unhandled exception: " << e.what();
        return EXIT_RESTART;
    }
}
