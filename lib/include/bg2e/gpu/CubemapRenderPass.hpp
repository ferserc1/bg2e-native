/*
 *    business grade graphic engine (bg2e engine)
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
#include <bg2e/gpu/Common.hpp>
#include <bg2e/math/base.hpp>

#include <functional>

namespace bg2e {
namespace gpu {

class CommandBuffer;
class CubeMap;

// Configuration for a single CubemapRenderPass::render() invocation.
struct CubemapRenderPassInfo {
    // Clear the color attachment of each face/mip before invoking the callback.
    bool  clear      = true;
    Color clearColor = Color(0.0f, 0.0f, 0.0f, 1.0f);

    // Transition the cubemap image to ColorAttachment before rendering, and to
    // ShaderReadOnly after rendering. Disable either one when the caller wants
    // to manage transitions explicitly (e.g. when chaining several passes).
    bool transitionBefore = true;
    bool transitionAfter  = true;

    // Perspective parameters used to build the per-face projection matrix.
    // The field of view is fixed at 90 degrees and the aspect ratio at 1.0,
    // as required to cover a single cubemap face.
    float nearPlane = 0.1f;
    float farPlane  = 1000.0f;
};

// Low-level helper that encapsulates the repetitive pattern of rendering into
// the six faces of a cubemap and, when the cubemap has mipmaps, into all of its
// mip levels.
//
// CubemapRenderPass does NOT own or create the cubemap: it must be created
// beforehand through the regular GPU resource API (Device::createCubeMap).
// The helper only drives the render iteration. All actual render content
// (pipeline binding, resource sets, push constants, draw calls, per-mip
// parameters, ...) lives inside the user callback.
//
// For each mip level and each cubemap face the helper:
//   - computes the cubemap projection matrix (90 deg FOV, 1:1 aspect);
//   - computes the face view matrix;
//   - calls cmd->beginRendering(cubemap, face, mipLevel);
//   - clears the color attachment when configured;
//   - invokes the callback with the current face/mip and matrices;
//   - calls cmd->endRendering().
//
// Viewport and scissor are derived automatically by the backend from the mip
// size (see CommandBuffer::bindPipeline / beginRendering), so the callback does
// not need to set them.
//
// Image transitions (ColorAttachment before, ShaderReadOnly after) are issued
// once around the whole iteration and can be toggled through the info struct.
class BG2E_API CubemapRenderPass {
public:
    // Callback invoked once per cubemap face and mip level. The projection and
    // view matrices correspond to the current face and are ready to be uploaded
    // to the shader (push constants, uniform buffer, ...).
    using RenderFunction = std::function<void(
        CommandBuffer*    cmd,
        CubemapFace       face,
        uint32_t          mipLevel,
        const glm::mat4&  projection,
        const glm::mat4&  view
    )>;

    static void render(
        CommandBuffer*               cmd,
        CubeMap*                     cubemap,
        const CubemapRenderPassInfo& info,
        const RenderFunction&        callback
    );
};

}
}
