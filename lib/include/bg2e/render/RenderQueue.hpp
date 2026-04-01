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

#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/math/base.hpp>
#include <bg2e/scene/Drawable.hpp>

#include <memory>
#include <vector>

namespace bg2e {
namespace render {

enum RenderQueueType {
    Opaque = 0,
    Transparent = 1,
    SolidTransparent = 2
};

template <typename DrawableT>
class RenderQueue {
public:
    struct Item
    {
        std::shared_ptr<render::vulkan::geo::Mesh> renderMesh;
        uint32_t submeshIndex;
        std::shared_ptr<render::MaterialBase> material;
        glm::mat4 worldMatrix;
    };

    void beginFrame();
    
    void enqueue(
        std::shared_ptr<render::vulkan::geo::Mesh> mesh,
        uint32_t submeshIndex,
        std::shared_ptr<render::MaterialBase> material,
        const glm::mat4 worldMatrix
    );
    
    inline const std::vector<Item>& opaqueQueue() const { return _opaqueQueue; }
    inline const std::vector<Item>& transparentQueue() const { return _transparentQueue; }
    inline const std::vector<Item>& solidTransparentQueue() const { return _solidTransparentQueue; }
    
    void render(
        RenderQueueType queueType,
        VkCommandBuffer cmd,
        VkPipelineLayout pipelineLayout,
        bg2e::scene::DrawableBase::DrawFunction fn,
        const glm::vec3& cameraPos
    );
    
    void cleanup();
    
protected:
    std::vector<Item> _opaqueQueue;
    std::vector<Item> _transparentQueue;
    std::vector<Item> _solidTransparentQueue;
    
};

}
}
