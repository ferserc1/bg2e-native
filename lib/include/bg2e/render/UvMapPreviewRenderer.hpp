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
#include <bg2e/geo/Mesh.hpp>
#include <bg2e/render/vulkan/common.hpp>

#include <cstdint>
#include <memory>

namespace bg2e {
namespace render {

class Engine;
class Texture;

namespace vulkan {
class Buffer;
}

// Renders a UV map (UV1 or UV2) of a CPU mesh into an internal square
// Vulkan image: triangle wireframes tinted per submesh over a dim coverage
// fill, plus the [0,1] atlas boundaries. The image is usable as a sampled
// texture through the render::Texture handle returned by texture().
// It never mutates the mesh and does not depend on bg2e::ui.
class BG2E_API UvMapPreviewRenderer {
public:
    UvMapPreviewRenderer(Engine * engine, uint32_t resolution = 512);
    ~UvMapPreviewRenderer();

    UvMapPreviewRenderer(const UvMapPreviewRenderer &) = delete;
    UvMapPreviewRenderer & operator=(const UvMapPreviewRenderer &) = delete;

    // Recreates the target image safely, waiting for pending GPU work.
    void setResolution(uint32_t resolution);
    inline uint32_t resolution() const { return _resolution; }

    // Renders the selected UV set (0 = UV1, 1 = UV2). Synchronous: submits the
    // work and waits for completion, leaving the image in
    // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
    void render(const geo::Mesh & mesh, uint32_t uvSet);

    // Sampled image of the last render() call, valid until the next
    // setResolution(). Usable wherever a render::Texture is accepted.
    inline Texture * texture() { return _texture.get(); }
    inline const Texture * texture() const { return _texture.get(); }

private:
    Engine * _engine;
    uint32_t _resolution;
    std::unique_ptr<Texture> _texture;
    VkImageLayout _targetLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkPipeline _trianglePipeline = VK_NULL_HANDLE;
    VkPipeline _linePipeline = VK_NULL_HANDLE;
    std::unique_ptr<vulkan::Buffer> _borderBuffer;

    void createTarget();
    void createResources();
    void destroyPipelines();
};

}
}
