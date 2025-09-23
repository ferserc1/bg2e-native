//
//  RenderQueue.hpp

#pragma once

#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/math/base.hpp>

#include <memory>
#include <vector>

namespace bg2e {
namespace render {

template <typename RenderMeshT>
class RenderQueue {
public:
    struct Item
    {
        std::vector<std::shared_ptr<RenderMeshT>> renderMesh;
        uint32_t submeshIndex;
        std::shared_ptr<render::MaterialBase> material;
        glm::mat4 worldMatrix;
    };

    inline void beginFrame()
    {
        _opaqueQueue.clear();
        _transparentQueue.clear();
    }
    
    inline void enqueue(
        std::shared_ptr<RenderMeshT> mesh,
        uint32_t submeshIndex,
        std::shared_ptr<render::MaterialBase> material,
        const glm::mat4 worldMatrix
    ) {
        auto & queue = material->materialAttributes().isTransparent() ? _transparentQueue : _opaqueQueue;
        queue.push({
            mesh,
            submeshIndex,
            material,
            worldMatrix
        });
    }
    
    inline const std::vector<Item>& opaqueQueue() const { return _opaqueQueue; }
    inline const std::vector<Item>& transparentQueue() const { return _transparentQueue; }
    
    inline void cleanup()
    {
        _opaqueQueue.clear();
        _opaqueQueue.clear();
    }
    
protected:
    std::vector<Item> _opaqueQueue;
    std::vector<Item> _transparentQueue;
    
};

}
}
