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

#include <cstdint>
#include <filesystem>
#include <string>

// Coordinate system of the source file. bg2e/glTF native is y_up.
enum class ImportCoordinateSystem {
    YUp,    // no conversion (glTF native)
    ZUp     // wrapper gets -90 deg rotation on X
};

// POD describing one pending scene import. Delivered by the HTTP worker
// thread to the main thread through the ImportServer queue. It has no
// dependency on httplib or on any scene type.
struct ImportRequest {
    uint64_t id = 0;                        // assigned by ImportServer
    std::string fileName;                   // informational / fallback node name
    std::filesystem::path filePath;         // absolute local path (validated)
    float unitsScale = 1.0f;                // meters per source unit
    ImportCoordinateSystem coordinateSystem = ImportCoordinateSystem::YUp;
};
