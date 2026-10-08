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

#include <bg2e/render/StandaloneBakeBatch.hpp>

#include <filesystem>
#include <iosfwd>
#include <string>

namespace lightmap_generator {

enum class Command { Model, Prefab };

struct Options {
    Command command = Command::Model;
    std::filesystem::path contextPath;
    std::filesystem::path modelPath;
    std::filesystem::path prefabPath;
    std::filesystem::path outputDirectory;
    bg2e::db::ImageFormat imageFormat = bg2e::db::ImageFormat::PNG;
    bg2e::render::StandaloneBakeBatch::Options batch;
    bool giBouncesSpecified = false;
    bool maxDistanceSpecified = false;
};

bool helpRequested(int argc, char** argv);
Options parseOptions(int argc, char** argv);
void validateOptions(const Options& options);
void printUsage(std::ostream& output);

}
