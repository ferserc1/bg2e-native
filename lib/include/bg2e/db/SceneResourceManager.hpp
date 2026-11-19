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
#include <bg2e/json/JsonNode.hpp>
#include <filesystem>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace bg2e::db {

// Shared by the loader and resource analysis. Reads references only, not assets.
struct SceneFileReferences {
    std::vector<std::filesystem::path> files;
    std::vector<std::filesystem::path> models;
    std::vector<std::string> warnings;

    bool operator==(const SceneFileReferences&) const = default;
};
BG2E_API SceneFileReferences sceneFileReferences(std::shared_ptr<json::JsonNode> node,
    const std::filesystem::path& basePath);

// Transient caller-supplied roots, never serialized into the scene document.
// Cache identities and generations are CPU values; file-only callers need no device.
struct SceneResourceProtection {
    SceneFileReferences roots;
    std::map<uint64_t, uint64_t> cacheGenerations;

    bool operator==(const SceneResourceProtection&) const = default;
};

struct SceneResourceFile {
    std::filesystem::path path;
    uintmax_t bytes = 0;
    uint64_t fingerprint = 0;
    std::filesystem::file_time_type modified{};

    bool operator==(const SceneResourceFile&) const = default;
};

struct SceneResourceAnalysis {
    std::filesystem::path scenePath;
    SceneResourceProtection protection;
    std::vector<std::filesystem::path> resources;
    std::vector<std::filesystem::path> externalResources;
    std::vector<SceneResourceFile> unusedFiles;
    std::vector<std::string> warnings;

    // Snapshot of local files and all dependencies, including missing ones.
    std::map<std::filesystem::path, SceneResourceFile> snapshot;
    std::vector<std::filesystem::path> symbolicLinks;
};

class SceneResourceManager;

// Opaque reviewed snapshot; execution never expands the deletion list.
class BG2E_API SceneCleanupPlan {
  public:
    const SceneResourceAnalysis& analysis() const
    {
        return _analysis;
    }

  private:
    SceneResourceAnalysis _analysis;
    bool _ignoreWarnings = false;
    friend class SceneResourceManager;
};

struct SceneCleanupResult {
    std::vector<std::filesystem::path> deleted;
    std::vector<std::string> errors;
};

// File-only operations: no scene graph, engine, device, or editor save state.
// The caller must serialize writers to the scene directory during execution.
class BG2E_API SceneResourceManager {
  public:
    static SceneResourceAnalysis analyze(const std::filesystem::path& scenePath);
    static SceneResourceAnalysis analyze(const std::filesystem::path& scenePath, const SceneResourceProtection& protection);
    // Revalidates the analysis. Warnings require explicit acknowledgement.
    // Fatal document/directory failures throw; dependency failures become warnings.
    static SceneCleanupPlan createCleanupPlan(const SceneResourceAnalysis& analysis, bool ignoreWarnings = false);
    static SceneCleanupPlan createCleanupPlan(const SceneResourceAnalysis& analysis,
        const SceneResourceProtection& protection, bool ignoreWarnings);
    // Reanalyzes before deleting; stale plans throw without deleting any file.
    // Individual removal failures are returned; successful removals are not rolled back.
    static SceneCleanupResult executeCleanup(const SceneCleanupPlan& plan);
    static SceneCleanupResult executeCleanup(const SceneCleanupPlan& plan, const SceneResourceProtection& protection);

    // Internal reload may add protection, never expand deletion. Every reviewed
    // cache generation and filesystem fingerprint must still match. Old roots
    // must be carried explicitly even when history/clipboard are retired.
    static SceneCleanupPlan rebindCleanupPlan(const SceneCleanupPlan& plan, const SceneResourceProtection& protection);
};

}
