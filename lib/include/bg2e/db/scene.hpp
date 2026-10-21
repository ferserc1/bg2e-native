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

#include <bg2e/scene/Scene.hpp>

#include <memory>
#include <string>
#include <filesystem>

namespace bg2e {

namespace db {

// Representation IO preserves unknown fields/components without constructing a
// graph or opening referenced assets. Neither operation exports GPU resources.
extern BG2E_API std::shared_ptr<json::JsonNode> readSceneDocument(const std::filesystem::path& filePath);
extern BG2E_API void writeSceneDocument(std::shared_ptr<json::JsonNode> document,
    const std::filesystem::path& filePath);
extern BG2E_API void saveSceneReferences(scene::Node* root, const std::filesystem::path& filePath,
    scene::SceneProgressCallback onProgress = nullptr);

// Builds a graph of CPU attributes and unresolved source references only.
extern BG2E_API std::shared_ptr<scene::Scene> loadScene(const std::filesystem::path& filePath,
    scene::SceneProgressCallback onProgress = nullptr);
extern BG2E_API std::shared_ptr<scene::Scene> loadScene(const std::filesystem::path& basePath,
    const std::string& fileName, scene::SceneProgressCallback onProgress = nullptr);
// Realizes mesh/material resources using only the supplied draw context.
extern BG2E_API std::shared_ptr<scene::Scene> loadScene(const std::filesystem::path& filePath,
    draw::Engine& engine, scene::SceneProgressCallback onProgress = nullptr);
extern BG2E_API std::shared_ptr<scene::Scene> loadScene(const std::filesystem::path& basePath,
    const std::string& fileName, draw::Engine& engine, scene::SceneProgressCallback onProgress = nullptr);

// Complete resource snapshot. Call at a stopped submission boundary. Unique
// sidecars are published first and the main document is replaced atomically last.
// Returns false and logs the error on failure; never propagates exceptions.
extern BG2E_API bool saveScene(scene::Node* root, draw::Engine& engine,
    const std::filesystem::path& path, scene::SceneProgressCallback progress = nullptr) noexcept;


}
}
