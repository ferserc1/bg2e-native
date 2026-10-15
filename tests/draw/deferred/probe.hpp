#pragma once

#include <bg2e/draw/all.hpp>
#include <bg2e/scene/Node.hpp>
#include <cmath>
#include <stdexcept>

namespace bg2e::draw::tests {
// Application-owned compatible subclass, retained outside the renderer.
class CountingGBuffer : public pipeline::GBuffer {
  public:
    using GBuffer::GBuffer;
    uint32_t opaquePasses = 0;
    uint32_t transparentPasses = 0;
    void record(const pipeline::PassContext& context) override
    {
        GBuffer::record(context);
        if (context.raster && context.raster->item)
        {
            ++transparentPasses;
        }
        else
        {
            ++opaquePasses;
        }
    }
};
// Call before acquisition with an initialized Engine and stopped producers.
// The identity camera sees three overlapping squares. Red opaque depth=0.5,
// green glass behind it=0.8, blue glass in front=0.2. The center must be purple
// in linear HDR: (0.5,0,0.5), never contaminated by the occluded green surface.
inline std::shared_ptr<scene::Node> deferredProbeScene(Engine& engine)
{
    auto root = std::make_shared<scene::Node>("deferred probe");
    const base::Color colors[] = {{1, 0, 0, 1}, {0, 1, 0, 0.5f}, {0, 0, 1, 0.5f}};
    const float depths[] = {0.5f, 0.8f, 0.2f};
    for (uint32_t i = 0; i < 3; ++i)
    {
        auto mesh = std::make_shared<Drawable::Mesh>();
        const glm::vec3 positions[] = {{-0.7f, -0.7f, 0}, {0.7f, -0.7f, 0}, {0.7f, 0.7f, 0}, {-0.7f, 0.7f, 0}};
        for (const auto& position : positions)
        {
            mesh->vertices.push_back({position, {0, 0, 1}, {0.5f, 0.5f}, {0.5f, 0.5f}, {1, 0, 0}});
        }
        mesh->indices = {0, 1, 2, 0, 2, 3};
        mesh->submeshes = {{0, 6}};
        auto drawable = std::make_shared<Drawable>();
        drawable->setMesh(mesh);
        drawable->setRayTracingEnabled(false);
        drawable->material().setAlbedo(colors[i]);
        drawable->material().setIsUnlit(true);
        drawable->material().setLightEmission(0);
        drawable->material().setAlphaCutoff(0);
        drawable->material().setIsTransparent(i != 0);
        drawable->material().setIsSolid(i == 2);
        drawable->setTransform(glm::translate(glm::mat4{1}, glm::vec3(0, 0, depths[i])));
        drawable->load(&engine);
        auto node = std::make_shared<scene::Node>("probe surface");
        node->addComponent(new scene::DrawableComponent(drawable));
        root->addChild(node);
    }
    return root;
}
inline void checkDeferredCenter(const std::vector<uint8_t>& rgba, gpu::Size2D extent)
{
    if (extent.isZero() || rgba.size() != size_t(extent.width) * extent.height * 4)
    {
        throw std::invalid_argument("Expected tightly packed RGBA8 readback");
    }
    const size_t center = (size_t(extent.height / 2) * extent.width + extent.width / 2) * 4;
    const float expected = std::pow(1.0f - std::exp(-0.5f), 1.0f / 2.2f) * 255.0f;
    if (std::abs(float(rgba[center]) - expected) > 5 || rgba[center + 1] > 3 ||
        std::abs(float(rgba[center + 2]) - expected) > 5 || rgba[center + 3] != 255)
    {
        throw std::runtime_error("Deferred alpha/depth composition or final color processing differs");
    }
}
}
