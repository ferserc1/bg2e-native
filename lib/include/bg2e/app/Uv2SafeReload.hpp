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

#include <functional>
#include <memory>
#include <string>

namespace bg2e {
namespace scene {
class Node;
}
namespace draw {
class DefaultRenderLoopDelegate;
}
namespace app {

struct SafeUpdateToken;

struct Uv2RegenerationResult {
    bool success = false;
    std::string message;
    uint32_t atlasWidth = 0;
    uint32_t atlasHeight = 0;
    uint32_t chartCount = 0;
    float utilization = 0.0f;
};

// Regenerates UV2 using an independent draw realization at a stopped delegate
// boundary. The returned weak cancellation token prevents stale publication.
class BG2E_API Uv2SafeReload {
  public:
    using CompletionCallback = std::function<void(const Uv2RegenerationResult&)>;

    // Draw-specific queued edit; retains no renderer pointer. Generation/root,
    // component/mesh identity and weak cancellation are checked before mutation.
    // beforeChange invalidates application-owned bake/source state at the drained
    // boundary (also on a later failure). Success replaces this node's Drawable
    // with an independent realization; completion must refresh selection/previews.
    static std::shared_ptr<SafeUpdateToken> regenerateDraw(
        std::weak_ptr<draw::DefaultRenderLoopDelegate> delegate,
        std::weak_ptr<scene::Node> targetNode,
        const geo::Uv2AtlasOptions& options = {},
        CompletionCallback onComplete = nullptr,
        std::function<void()> beforeChange = {});
};

}
}
