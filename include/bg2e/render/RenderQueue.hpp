//
//  RenderQueue.hpp

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
