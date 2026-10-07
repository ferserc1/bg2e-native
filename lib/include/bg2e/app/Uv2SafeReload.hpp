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
namespace app {

struct SafeUpdateToken;

struct Uv2RegenerationResult
{
    bool success = false;
    std::string message;
    uint32_t atlasWidth = 0;
    uint32_t atlasHeight = 0;
    uint32_t chartCount = 0;
    float utilization = 0.0f;
};

// Narrow application-facing helper that regenerates the UV2 atlas of a loaded
// scene Drawable and reloads its GPU resources safely. The CPU work
// (geo::GenerateUv2AtlasModifier::apply) and Drawable::reload() run inside
// MainLoop::safeUpdateScene, i.e. after Engine::device().waitIdle(), so no
// in-flight GPU resource is modified.
//
// Usage:
//   _token = Uv2SafeReload::regenerate(weakNode, options,
//       [this](const Uv2RegenerationResult & r) { ... });
//
// The returned token cancels the pending work when the caller releases its
// last reference (scene swap, window destruction). For a live node, the
// callback runs on the main thread after the reload or a reported failure. If
// the weak node is already expired at entry, the failure callback runs
// immediately on the calling thread.
// Submesh count and material attributes are preserved by Drawable::reload();
// the helper verifies the submesh count and reports failure otherwise. After a
// successful reload, callers must invalidate any active bake accumulation and
// refresh UV previews (UvMapPreview::refresh) through the completion callback.
class BG2E_API Uv2SafeReload {
public:
    using CompletionCallback = std::function<void(const Uv2RegenerationResult &)>;

    // Schedules regeneration of the standard scene::Drawable directly attached
    // to targetNode. Returns the cancellation token (never null). The callback
    // runs on success or failure unless the token is destroyed before queued
    // work begins. A node that expires before execution is reported as failure.
    static std::shared_ptr<SafeUpdateToken> regenerate(
        std::weak_ptr<scene::Node> targetNode,
        const geo::Uv2AtlasOptions & options = {},
        CompletionCallback onComplete = nullptr);
};

}
}
