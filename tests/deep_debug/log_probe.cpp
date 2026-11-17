/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/base/Log.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using bg2e::base::Log;

namespace {
void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 3, "Usage: log_probe disabled|stress|write-failure|abort|hold|open DIRECTORY");
        const std::string mode = argv[1];
        const std::filesystem::path directory = argv[2];
        const bool debugBuild = Log::isDebug();
        int evaluations = 0;
        bg2e_deep_trace("test", "disabled", ++evaluations);
        require(evaluations == 0, "Disabled trace evaluated its payload");
        if (mode == "disabled")
        {
            Log::shutdownDeepDebug();
            require(!std::filesystem::exists(directory / "bg2e-deep-debug.log"), "Disabled logger created a file");
            return 0;
        }
        Log::initializeDeepDebug(directory);
        require(Log::deepDebugEnabled(), "Session did not activate");
        require(Log::isDebug() == debugBuild, "Runtime tracing changed build debug policy");
        if (mode == "hold")
        {
            bg2e_deep_trace("test", "ownership.held", "ready");
            std::cout << "READY" << std::endl;
            std::cin.get();
        }
        else if (mode == "abort")
        {
            bg2e_deep_trace("test", "abort.last", "flushed before abort");
            std::abort();
        }
        else if (mode == "stress" || mode == "write-failure")
        {
            if (mode == "write-failure")
            {
                std::filesystem::create_directory(directory / "bg2e-deep-debug.log.tmp");
            }
            bg2e_deep_trace("test", "oldest", "must be removed by retention");
            std::vector<std::thread> workers;
            for (int worker = 0; worker < 4; ++worker)
            {
                workers.emplace_back([worker]
                {
                    const std::string payload(1024, static_cast<char>('a' + worker));
                    for (int index = 0; index < 7000; ++index)
                    {
                        bg2e_deep_trace("test", "concurrent", "worker=" << worker << " index=" << index << " payload=" << payload << " end=complete");
                    }
                });
            }
            for (auto& worker : workers)
            {
                worker.join();
            }
            if (mode == "write-failure")
            {
                require(!Log::deepDebugEnabled(), "Output failure did not disable tracing");
            }
            bg2e_log_warning << "warning-capture-" << 42 << bg2e_log_end;
            bg2e_log_error << "error-capture-" << 43 << bg2e_log_end;
            bg2e_deep_trace("test", "huge", std::string(65536, 'z'));
            for (int index = 0; index < 100; ++index)
            {
                Log::trace("test", "repeat", [](std::ostream& output) { output << "same warning"; }, true);
            }
            Log::trace("test", "repeat", [](std::ostream& output) { output << "distinct warning"; }, true);
            bg2e_deep_trace("test", "newest", "must survive retention");
        }
        else
        {
            require(mode == "open", "Unknown probe mode");
        }
        Log::shutdownDeepDebug();
        bg2e_deep_trace("test", "after.shutdown", ++evaluations);
        require(evaluations == 0, "Late trace evaluated its payload after shutdown");
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        Log::shutdownDeepDebug();
        return 2;
    }
}
