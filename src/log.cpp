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

#include <metricq/logger.hpp>

class Logger : public metricq::Logger
{
    using Log = combinator_log::Log;

public:
    void trace(const std::string& msg) override
    {
        Log::trace("metricq") << msg;
    }

    void debug(const std::string& msg) override
    {
        Log::debug("metricq") << msg;
    }

    void notice(const std::string& msg) override
    {
        Log::info("metricq") << msg;
    }

    void info(const std::string& msg) override
    {
        Log::info("metricq") << msg;
    }

    void warn(const std::string& msg) override
    {
        Log::warn("metricq") << msg;
    }

    void error(const std::string& msg) override
    {
        Log::error("metricq") << msg;
    }

    void fatal(const std::string& msg) override
    {
        Log::fatal("metricq") << msg;
    }
};

namespace combinator_log
{
void initialize()
{
    metricq::make_logger<Logger>();
}
} // namespace combinator_log
