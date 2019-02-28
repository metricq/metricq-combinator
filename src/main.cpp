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
#include "log.hpp"
#include "transformer.hpp"

#include <nitro/broken_options/parser.hpp>

using Log = combinator_log::Log;

nitro::broken_options::options parse_options(int argc, const char* argv[])
{
    using combinator_log::set_severity;
    set_severity(nitro::log::severity_level::debug);

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

        combinator_log::initialize();

        if (options.given("help"))
        {
            parser.usage();
            std::exit(EXIT_SUCCESS);
        }

        if (options.given("trace"))
        {
            combinator_log::set_severity(nitro::log::severity_level::trace);
        }
        else if (options.given("verbose"))
        {
            combinator_log::set_severity(nitro::log::severity_level::debug);
        }
        else if (options.given("quiet"))
        {
            combinator_log::set_severity(nitro::log::severity_level::warn);
        }

        return options;
    }
    catch (nitro::broken_options::parsing_error& e)
    {
        Log::warn() << "Error parsing options: " << e.what();
        parser.usage();
        std::exit(EXIT_FAILURE);
    }
    catch (std::exception& e)
    {
        Log::error() << "Unhandled exception: " << e.what();
        parser.usage();
        std::exit(EXIT_FAILURE);
    }
}

int main(int argc, const char* argv[])
{
    auto options = parse_options(argc, argv);

    try
    {
        Transformer transformer{
            options.get("server"),
            options.get("token"),
        };

        Log::info() << "starting main loop...";
        transformer.main_loop();
        Log::info() << "stopped.";
    }
    catch (const CombinedMetric::ParseError& e)
    {
        Log::error() << "Error parsing options: " << e.what();
    }
}
