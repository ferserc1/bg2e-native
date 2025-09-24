//
//  RenderQueue.cpp

#include <bg2e/render/RenderQueue.hpp>
#include <bg2e/render/vulkan/geo/Mesh.hpp>
#include <algorithm>

namespace bg2e {
namespace render {

template <typename DrawableT>
void RenderQueue<DrawableT>::beginFrame()
{
    _opaqueQueue.clear();
    _transparentQueue.clear();
    _solidTransparentQueue.clear();
}

template <typename DrawableT>
void RenderQueue<DrawableT>::enqueue(
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

template <typename DrawableT>
void RenderQueue<DrawableT>::render(
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

template <typename DrawableT>
void RenderQueue<DrawableT>::cleanup()
{
    _opaqueQueue.clear();
    _transparentQueue.clear();
    _solidTransparentQueue.clear();
}

// DrawableP
template void RenderQueue<scene::DrawableP>::beginFrame();
template void RenderQueue<scene::DrawableP>::enqueue(std::shared_ptr<render::vulkan::geo::Mesh> mesh, uint32_t submeshIndex, std::shared_ptr<render::MaterialBase> material, const glm::mat4 worldMatrix);
template void RenderQueue<scene::DrawableP>::render(RenderQueueType queueType, VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, bg2e::scene::DrawableBase::DrawFunction fn, const glm::vec3& cameraPos);
template void RenderQueue<scene::DrawableP>::cleanup();

// DrawablePN
template void RenderQueue<scene::DrawablePN>::beginFrame();
template void RenderQueue<scene::DrawablePN>::enqueue(std::shared_ptr<render::vulkan::geo::Mesh> mesh, uint32_t submeshIndex, std::shared_ptr<render::MaterialBase> material, const glm::mat4 worldMatrix);
template void RenderQueue<scene::DrawablePN>::render(RenderQueueType queueType, VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, bg2e::scene::DrawableBase::DrawFunction fn, const glm::vec3& cameraPos);
template void RenderQueue<scene::DrawablePN>::cleanup();

// DrawablePC
template void RenderQueue<scene::DrawablePC>::beginFrame();
template void RenderQueue<scene::DrawablePC>::enqueue(std::shared_ptr<render::vulkan::geo::Mesh> mesh, uint32_t submeshIndex, std::shared_ptr<render::MaterialBase> material, const glm::mat4 worldMatrix);
template void RenderQueue<scene::DrawablePC>::render(RenderQueueType queueType, VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, bg2e::scene::DrawableBase::DrawFunction fn, const glm::vec3& cameraPos);
template void RenderQueue<scene::DrawablePC>::cleanup();

// DrawablePU
template void RenderQueue<scene::DrawablePU>::beginFrame();
template void RenderQueue<scene::DrawablePU>::enqueue(std::shared_ptr<render::vulkan::geo::Mesh> mesh, uint32_t submeshIndex, std::shared_ptr<render::MaterialBase> material, const glm::mat4 worldMatrix);
template void RenderQueue<scene::DrawablePU>::render(RenderQueueType queueType, VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, bg2e::scene::DrawableBase::DrawFunction fn, const glm::vec3& cameraPos);
template void RenderQueue<scene::DrawablePU>::cleanup();

// DrawablePNU
template void RenderQueue<scene::DrawablePNU>::beginFrame();
template void RenderQueue<scene::DrawablePNU>::enqueue(std::shared_ptr<render::vulkan::geo::Mesh> mesh, uint32_t submeshIndex, std::shared_ptr<render::MaterialBase> material, const glm::mat4 worldMatrix);
template void RenderQueue<scene::DrawablePNU>::render(RenderQueueType queueType, VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, bg2e::scene::DrawableBase::DrawFunction fn, const glm::vec3& cameraPos);
template void RenderQueue<scene::DrawablePNU>::cleanup();

// DrawablePNC
template void RenderQueue<scene::DrawablePNC>::beginFrame();
template void RenderQueue<scene::DrawablePNC>::enqueue(std::shared_ptr<render::vulkan::geo::Mesh> mesh, uint32_t submeshIndex, std::shared_ptr<render::MaterialBase> material, const glm::mat4 worldMatrix);
template void RenderQueue<scene::DrawablePNC>::render(RenderQueueType queueType, VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, bg2e::scene::DrawableBase::DrawFunction fn, const glm::vec3& cameraPos);
template void RenderQueue<scene::DrawablePNC>::cleanup();

// DrawablePNUC
template void RenderQueue<scene::DrawablePNUC>::beginFrame();
template void RenderQueue<scene::DrawablePNUC>::enqueue(std::shared_ptr<render::vulkan::geo::Mesh> mesh, uint32_t submeshIndex, std::shared_ptr<render::MaterialBase> material, const glm::mat4 worldMatrix);
template void RenderQueue<scene::DrawablePNUC>::render(RenderQueueType queueType, VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, bg2e::scene::DrawableBase::DrawFunction fn, const glm::vec3& cameraPos);
template void RenderQueue<scene::DrawablePNUC>::cleanup();

// DrawablePNUT
template void RenderQueue<scene::DrawablePNUT>::beginFrame();
template void RenderQueue<scene::DrawablePNUT>::enqueue(std::shared_ptr<render::vulkan::geo::Mesh> mesh, uint32_t submeshIndex, std::shared_ptr<render::MaterialBase> material, const glm::mat4 worldMatrix);
template void RenderQueue<scene::DrawablePNUT>::render(RenderQueueType queueType, VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, bg2e::scene::DrawableBase::DrawFunction fn, const glm::vec3& cameraPos);
template void RenderQueue<scene::DrawablePNUT>::cleanup();

// DrawablePNUUT
template void RenderQueue<scene::DrawablePNUUT>::beginFrame();
template void RenderQueue<scene::DrawablePNUUT>::enqueue(std::shared_ptr<render::vulkan::geo::Mesh> mesh, uint32_t submeshIndex, std::shared_ptr<render::MaterialBase> material, const glm::mat4 worldMatrix);
template void RenderQueue<scene::DrawablePNUUT>::render(RenderQueueType queueType, VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, bg2e::scene::DrawableBase::DrawFunction fn, const glm::vec3& cameraPos);
template void RenderQueue<scene::DrawablePNUUT>::cleanup();

}
}
