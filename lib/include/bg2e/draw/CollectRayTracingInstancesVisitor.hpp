#pragma once

#include <bg2e/draw/RenderQueueVisitor.hpp>
#include <bg2e/gpu/RayTracingScene.hpp>

namespace bg2e::draw {
struct RayTracingCollectionOptions {
    bool includeTransparent = false;
    bool includeSolidTransparent = false;
    uint32_t maxInstances = 0x00ffffffu;
};
class BG2E_API CollectRayTracingInstancesVisitor {
  public:
    struct Instance {
        RenderQueue::Item item; // Retains raster buffers, BLAS, textures and identity.
        gpu::RayTracingInstance instance;
    };
    void collect(const RenderQueue& queue, RayTracingCollectionOptions options = {});
    void collect(scene::Node* root, Engine& engine, RayTracingCollectionOptions options = {});
    const std::vector<Instance>& instances() const;
    // Target scene borrows meshes; retain this collection through GPU completion.
    void populate(gpu::RayTracingScene& scene) const;
  private:
    Engine* _engine = nullptr;
    std::vector<Instance> _instances;
};
}
