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

#include <bg2e/gpu/CubemapRenderPass.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/CubeMap.hpp>
#include <bg2e/gpu/Image.hpp>

#include <array>
#include <stdexcept>

namespace bg2e {
namespace gpu {

namespace {

// Eye/center/up for each cubemap face, in the canonical face order defined by
// CubemapFace (PositiveX, NegativeX, PositiveY, NegativeY, PositiveZ,
// NegativeZ). The eye sits at the origin for every face.
struct FaceCamera {
    glm::vec3 center;
    glm::vec3 up;
};

constexpr std::array<FaceCamera, 6> kFaceCameras = {{
    { glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f) }, // +X
    { glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f) }, // -X
    { glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f) }, // +Y
    { glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f) }, // -Y
    { glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f) }, // +Z
    { glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f) }, // -Z
}};

}

void CubemapRenderPass::render(
    CommandBuffer*               cmd,
    CubeMap*                     cubemap,
    const CubemapRenderPassInfo& info,
    const RenderFunction&        callback
)
{
    if (!cmd)
    {
        throw std::runtime_error("gpu::CubemapRenderPass::render: command buffer is null");
    }
    if (!cubemap || !cubemap->isValid())
    {
        throw std::runtime_error("gpu::CubemapRenderPass::render: cubemap is null or not valid");
    }
    if (!callback)
    {
        throw std::runtime_error("gpu::CubemapRenderPass::render: render callback is empty");
    }

    // Single 90 degree, 1:1 aspect projection shared by all faces and mips.
    const glm::mat4 projection = glm::perspective(
        glm::radians(90.0f), 1.0f, info.nearPlane, info.farPlane
    );

    // A single transition covers every face and mip level of the image.
    if (info.transitionBefore)
    {
        cmd->transition(cubemap->image(), ImageLayout::ColorAttachment);
    }

    const uint32_t mipLevels = cubemap->mipLevels();
    for (uint32_t mipLevel = 0; mipLevel < mipLevels; ++mipLevel)
    {
        for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex)
        {
            const auto&     fc   = kFaceCameras[faceIndex];
            const glm::mat4 view = glm::lookAt(glm::vec3(0.0f), fc.center, fc.up);
            const auto      face = static_cast<CubemapFace>(faceIndex);

            cmd->beginRendering(cubemap, face, mipLevel);
            if (info.clear)
            {
                cmd->clearColor(0, info.clearColor);
            }

            callback(cmd, face, mipLevel, projection, view);

            cmd->endRendering();
        }
    }

    if (info.transitionAfter)
    {
        cmd->transition(cubemap->image(), ImageLayout::ShaderReadOnly);
    }
}

}
}
