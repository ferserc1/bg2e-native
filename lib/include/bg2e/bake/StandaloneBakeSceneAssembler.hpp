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
#include <bg2e/geo/GenerateUv2AtlasModifier.hpp>
#include <bg2e/geo/UvAtlasValidator.hpp>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace bg2e::draw { class Engine; }
namespace bg2e::scene { class Node; class Scene; }
namespace bg2e::bake {
// Builds a private candidate; no current context/job or caller scene is mutated.
// Call on a stopped producer thread. Borrowed Engine and Device outlive every
// Assembly/target and any resources exported from them. No RT capability is
// required for assembly; required bake effects are checked by createBaker.
class BG2E_API StandaloneBakeSceneAssembler final {
  public:
    struct Target {
        std::shared_ptr<scene::Node> node;
        std::filesystem::path inputSourcePath;
        std::string outputIdentity; // Stable hierarchy/name/sibling path, never a node/pick/table ID.
        geo::UvAtlasValidation uv2;
        std::optional<geo::Uv2AtlasResult> generatedUv2;
    };
    struct Assembly {
        std::shared_ptr<scene::Scene> scene; // Context sources plus attached target subtree.
        std::shared_ptr<scene::Node> targetRoot;
        std::filesystem::path contextSourcePath, inputSourcePath;
        std::vector<Target> targets; // Enabled standard geometry below targetRoot only.
        // Known standard input dependencies, canonical and unique. Custom external
        // component resources need their own discovery/export contract in batch IO.
        std::vector<std::filesystem::path> sourcePaths;
    };

    explicit StandaloneBakeSceneAssembler(draw::Engine& engine);

    // BG2/VWGLB or OBJ model, identity placement. JSON prefab accepts either a
    // scene array or a single serialized Node; no camera is required. Context
    // must be a scene document. Unknown components follow shared schema policy.
    Assembly assembleModel(const std::filesystem::path& contextJson,
        const std::filesystem::path& model,
        std::optional<geo::Uv2AtlasOptions> uv2Options = std::nullopt,
        bool loadTargetGpuResources = true) const;
    Assembly assemblePrefab(const std::filesystem::path& contextJson,
        const std::filesystem::path& prefabJson,
        std::optional<geo::Uv2AtlasOptions> uv2Options = std::nullopt,
        bool loadTargetGpuResources = true) const;

  private:
    Assembly assemble(const std::filesystem::path& contextPath,
        const std::filesystem::path& inputPath, std::shared_ptr<scene::Node> targetRoot,
        std::optional<geo::Uv2AtlasOptions> uv2Options, bool loadTargetGpuResources) const;
    void requireEngine() const;

    draw::Engine* _engine;
    uint64_t _deviceIdentity;
};
}
