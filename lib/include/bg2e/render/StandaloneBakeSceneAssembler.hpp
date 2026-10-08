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

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace bg2e {
namespace scene {
class Node;
class Scene;
}
namespace render {

class Engine;

class BG2E_API StandaloneBakeSceneAssembler final {
public:
    struct Target {
        std::shared_ptr<scene::Node> node;
        // The .bg2 file backing this target's Drawable (or the model input).
        std::filesystem::path inputSourcePath;
        std::string outputIdentity;
    };

    struct Assembly {
        std::shared_ptr<scene::Scene> scene;
        std::filesystem::path contextSourcePath;
        // The model .bg2 or prefab JSON that was assembled.
        std::filesystem::path inputSourcePath;
        std::vector<Target> targets;
    };

    explicit StandaloneBakeSceneAssembler(Engine* engine);

    [[nodiscard]] Assembly assembleModel(
        const std::filesystem::path& contextJson,
        const std::filesystem::path& modelBg2,
        std::optional<geo::Uv2AtlasOptions> uv2Options = std::nullopt) const;

    [[nodiscard]] Assembly assembleModel(
        const std::filesystem::path& contextJson,
        const std::filesystem::path& modelBg2,
        std::optional<geo::Uv2AtlasOptions> uv2Options,
        bool loadTargetGpuResources) const;

    [[nodiscard]] Assembly assemblePrefab(
        const std::filesystem::path& contextJson,
        const std::filesystem::path& prefabJson,
        std::optional<geo::Uv2AtlasOptions> uv2Options = std::nullopt) const;

    [[nodiscard]] Assembly assemblePrefab(
        const std::filesystem::path& contextJson,
        const std::filesystem::path& prefabJson,
        std::optional<geo::Uv2AtlasOptions> uv2Options,
        bool loadTargetGpuResources) const;

private:
    Engine* _engine = nullptr;
};

}
}
