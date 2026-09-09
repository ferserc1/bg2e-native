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

#include <bg2e/common.hpp>
#include <filesystem>

namespace bg2e::db {

enum class SceneArtifactType { Scene, Prefab };

struct SceneSaveTarget {
    std::filesystem::path path;
    bool pathChanged = false;
    bool fileExists = false;
};

// Naming validation only: does not require an existing file or parse JSON.
BG2E_API bool isValidScenePath(const std::filesystem::path& jsonPath,
    SceneArtifactType type = SceneArtifactType::Scene);

// Append .json if necessary, and for Scene enforce Name/Name.json.
// Throws on an invalid name. Does not change the filesystem.
BG2E_API std::filesystem::path resolveSceneSavePath(const std::filesystem::path& selectedPath,
    SceneArtifactType type = SceneArtifactType::Scene);

// Resolve and inspect the destination without creating anything or displaying UI.
// Throws on inaccessible paths, conflicting directories/files, or a final symlink.
BG2E_API SceneSaveTarget inspectSceneSaveTarget(const std::filesystem::path& selectedPath,
    SceneArtifactType type = SceneArtifactType::Scene);

}
