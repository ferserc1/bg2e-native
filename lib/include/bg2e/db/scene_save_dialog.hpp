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

#include <bg2e/db/scene_path.hpp>

namespace bg2e::db {

enum class SceneSavePathSource { Direct, NativeSaveDialog };
enum class SceneOverwritePolicy { Confirm, Allow };

// Resolve and request native overwrite confirmation when required. No ImGui or
// rendering context is needed. Returns empty on cancellation; throws on error.
// NativeSaveDialog is valid only for a path returned by an accepted save dialog.
// Allow skips only our overwrite prompt; destination validation still runs.
// No directories are created and nothing is written by either helper.
BG2E_API std::filesystem::path confirmSceneSavePath(
    const std::filesystem::path& selectedPath,
    SceneSavePathSource source = SceneSavePathSource::Direct,
    SceneArtifactType type = SceneArtifactType::Scene,
    SceneOverwritePolicy overwritePolicy = SceneOverwritePolicy::Confirm);

BG2E_API std::filesystem::path getSceneSavePath(
    SceneArtifactType type = SceneArtifactType::Scene);

}
