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

#pragma once

#include <bg2e/bake/StandaloneBakeBatch.hpp>
#include <bg2e/draw/EngineConfig.hpp>
#include <bg2e/draw/SettingsCodec.hpp>
#include <iosfwd>

namespace draw_lightmap_generator {
enum class Command
{
    Model, Prefab
};
struct Options {
    Command command = Command::Model;
    std::filesystem::path contextPath, inputPath, outputDirectory, configPath;
    bg2e::draw::EngineConfig engine;
    bg2e::bake::StandaloneBakeBatch::Options batch;
    bool generateUv2 = true;
    uint32_t uv2Padding = 4;
    std::vector<bg2e::draw::SettingsIssue> diagnostics;
};

bool helpRequested(int argc, char** argv);
Options parseOptions(int argc, char** argv);
void validateOptions(const Options& options);
void printUsage(std::ostream& output);
}
