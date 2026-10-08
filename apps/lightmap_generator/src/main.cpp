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

#include <bg2e.hpp>

#include "CommandLine.hpp"

#include <csignal>
#include <exception>
#include <iostream>

namespace {

volatile std::sig_atomic_t cancelRequested = 0;

void handleInterrupt(int)
{
    cancelRequested = 1;
}

int runBatch(bg2e::render::Engine& engine, const lightmap_generator::Options& options)
{
    bg2e::render::StandaloneBakeSceneAssembler assembler(&engine);
    auto assembly = options.command == lightmap_generator::Command::Model
        ? assembler.assembleModel(options.contextPath, options.modelPath, std::nullopt, false)
        : assembler.assemblePrefab(options.contextPath, options.prefabPath, std::nullopt, false);

    std::signal(SIGINT, handleInterrupt);
    auto progress = [](const bg2e::render::StandaloneBakeBatch::Progress& value) {
        std::cout << "\rBaking target " << (value.targetIndex + 1) << "/" << value.targetCount
                  << ": " << value.completedFrames << "/" << value.accumulationFrames << " samples"
                  << std::flush;
        if (value.completedFrames == value.accumulationFrames)
        {
            std::cout << '\n';
        }
        return cancelRequested == 0;
    };
    auto warning = [](const bg2e::render::StandaloneBakeBatch::SkippedTarget& target) {
        std::cerr << "Warning: skipped target '" << target.identity << "': " << target.reason << '\n';
    };

    const auto result = bg2e::render::StandaloneBakeBatch::run(
        &engine,
        assembly,
        options.outputDirectory,
        options.imageFormat,
        options.batch,
        progress,
        warning);
    if (result.cancelled)
    {
        std::cerr << '\n';
    }

    std::cout << "Baked " << result.bakedCount << " target(s); skipped "
              << result.skippedCount << " target(s)";
    if (result.cancelled)
    {
        std::cout << "; cancelled";
    }
    std::cout << ".\n";
    return result.cancelled ? 130 : 0;
}

}

int main(int argc, char** argv)
{
    if (lightmap_generator::helpRequested(argc, argv))
    {
        lightmap_generator::printUsage(std::cout);
        return 0;
    }

    lightmap_generator::Options options;
    try
    {
        options = lightmap_generator::parseOptions(argc, argv);
        lightmap_generator::validateOptions(options);
    }
    catch (const std::exception& error)
    {
        std::cerr << "lightmap_generator: " << error.what() << '\n';
        lightmap_generator::printUsage(std::cerr);
        return 2;
    }

    bg2e::render::Engine engine;
    try
    {
        engine.init();
    }
    catch (const std::exception& error)
    {
        std::cerr << "lightmap_generator: could not initialize the headless engine: "
                  << error.what() << '\n';
        return 1;
    }

    int result = 0;
    try
    {
        result = runBatch(engine, options);
    }
    catch (const std::exception& error)
    {
        std::cerr << "lightmap_generator: " << error.what() << '\n';
        result = 1;
    }

    try
    {
        engine.cleanup();
    }
    catch (const std::exception& error)
    {
        std::cerr << "lightmap_generator: engine cleanup failed: " << error.what() << '\n';
        if (result == 0) result = 1;
    }
    return result;
}
