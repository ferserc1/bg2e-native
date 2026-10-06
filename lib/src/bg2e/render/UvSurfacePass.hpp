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

#include <bg2e/render/gbuffer/GBufferManager.hpp>
#include <bg2e/scene/Drawable.hpp>

#include <memory>
#include <vector>

namespace bg2e {
namespace render {

class UvSurfacePass {
public:
    struct Diagnostic {
        VkExtent2D extent{};
        std::vector<uint32_t> submeshIndices;
        std::vector<uint32_t> validTexels;
    };

    UvSurfacePass(Engine* engine, VkExtent2D extent);
    ~UvSurfacePass();

    UvSurfacePass(const UvSurfacePass&) = delete;
    UvSurfacePass& operator=(const UvSurfacePass&) = delete;

    void record(VkCommandBuffer cmd,
                const scene::Drawable& drawable,
                const glm::mat4& nodeWorld,
                uint32_t frameSlot);

    [[nodiscard]] GBufferManager& manager(uint32_t frameSlot);
    [[nodiscard]] const GBufferManager& manager(uint32_t frameSlot) const;
    [[nodiscard]] Diagnostic readDiagnostic(uint32_t frameSlot) const;

private:
    Engine* _engine;
    VkExtent2D _extent;
    std::vector<std::unique_ptr<GBufferManager>> _managers;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkPipeline _pipeline = VK_NULL_HANDLE;

    void createPipeline();
};

}
}
