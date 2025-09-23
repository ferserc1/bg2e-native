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

template <typename RenderMeshT>
class RenderQueue {
public:
    struct Item
    {
        std::shared_ptr<render::vulkan::geo::Mesh> renderMesh;
        uint32_t submeshIndex;
        std::shared_ptr<render::MaterialBase> material;
        glm::mat4 worldMatrix;
    };

    inline void beginFrame()
    {
        _opaqueQueue.clear();
        _transparentQueue.clear();
        _solidTransparentQueue.clear();
    }
    
    inline void enqueue(
        std::shared_ptr<render::vulkan::geo::Mesh> mesh,
        uint32_t submeshIndex,
        std::shared_ptr<render::MaterialBase> material,
        const glm::mat4 worldMatrix
    ) {
        if (material->materialAttributes().isSolid() && material->materialAttributes().isTransparent()) {
            _solidTransparentQueue.push_back({
                mesh,
                submeshIndex,
                material,
                worldMatrix
            });
        } else if (material->materialAttributes().isTransparent()) {
            _transparentQueue.push_back({
                mesh,
                submeshIndex,
                material,
                worldMatrix
            });
        } else {
            _opaqueQueue.push_back({
                mesh,
                submeshIndex,
                material,
                worldMatrix
            });
        }
    }
    
    inline const std::vector<Item>& opaqueQueue() const { return _opaqueQueue; }
    inline const std::vector<Item>& transparentQueue() const { return _transparentQueue; }
    inline const std::vector<Item>& solidTransparentQueue() const { return _solidTransparentQueue; }
    
    void render(
        RenderQueueType queueType,
        VkCommandBuffer cmd,
        VkPipelineLayout pipelineLayout,
        bg2e::scene::DrawableBase::DrawFunction fn,
        const glm::vec3& cameraPos
    ) {
        std::vector<Item>* queue = nullptr;
        switch(queueType) {
            case RenderQueueType::Opaque:
                queue = &_opaqueQueue; break;
            case RenderQueueType::Transparent:
                queue = &_transparentQueue; break;
            case RenderQueueType::SolidTransparent:
                queue = &_solidTransparentQueue; break;
        }
        if (!queue) return;

        if (queueType == RenderQueueType::Transparent || queueType == RenderQueueType::SolidTransparent)
        {
            std::sort(queue->begin(), queue->end(),
                [&](const Item& a, const Item& b) {
                    glm::vec3 posA = glm::vec3(a.worldMatrix[3]);
                    glm::vec3 posB = glm::vec3(b.worldMatrix[3]);
                    float distA = glm::length2(cameraPos - posA);
                    float distB = glm::length2(cameraPos - posB);
                    return distA > distB;
                }
            );
        }
        for (auto & queueItem : *queue)
        {
            std::vector<VkDescriptorSet> ds = fn(queueItem.material.get(), queueItem.worldMatrix, queueItem.submeshIndex);
            queueItem.renderMesh->drawSubmesh(cmd, pipelineLayout, ds, queueItem.submeshIndex);
        }
    }
    
    inline void cleanup()
    {
        _opaqueQueue.clear();
        _transparentQueue.clear();
        _solidTransparentQueue.clear();
    }
    
protected:
    std::vector<Item> _opaqueQueue;
    std::vector<Item> _transparentQueue;
    std::vector<Item> _solidTransparentQueue;
    
};

}
}
