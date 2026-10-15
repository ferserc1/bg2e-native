#pragma once

#include <bg2e/draw/all.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/gpu/Device.hpp>
#include <limits>
#include <stdexcept>

namespace bg2e::draw::tests {
// Stopped initialized Engine, no active frame. The lead compiles/runs this.
inline void runShadowTableProbe(Engine& engine)
{
    auto mesh = std::make_shared<Drawable::Mesh>();
    for (uint32_t i = 0; i < 6; ++i)
    {
        mesh->vertices.push_back({{float(i % 3), float(i / 3), float(i == 2 || i == 5)}, {0, 0, -1},
            {float(i) * 0.1f, 0.25f}, {float(i) * 0.1f, 0.75f}, {1, 0, 0}});
    }
    mesh->indices = {0, 1, 2, 3, 4, 5};
    mesh->submeshes = {{0, 3}, {3, 3}};
    auto skipped = std::make_shared<Drawable>();
    skipped->setMesh(mesh);
    skipped->setRayTracingEnabled(false);
    skipped->load(&engine);
    auto caster = std::make_shared<Drawable>();
    caster->setMesh(mesh);
    caster->material(1).setAlbedo(base::Color(1, 1, 1, 0.6f));
    caster->material(1).setAlbedoUVSet(1);
    caster->material(1).setAlbedoScale(glm::vec2(2, 3));
    caster->material(1).setAlphaCutoff(0.4f);
    caster->load(&engine);
    auto root = std::make_shared<scene::Node>("query table probe");
    auto ignoredNode = std::make_shared<scene::Node>("no tracing");
    ignoredNode->addComponent(new scene::DrawableComponent(skipped));
    root->addChild(ignoredNode);
    auto casterNode = std::make_shared<scene::Node>("two submeshes");
    casterNode->addComponent(new scene::DrawableComponent(caster));
    root->addChild(casterNode);
    RenderQueue queue;
    RenderQueueVisitor visitor;
    visitor.enqueue(root.get(), engine, queue);
    CollectRayTracingInstancesVisitor collection;
    collection.collect(queue);
    const auto table = pipeline::buildShadowQueryTable(collection);
    if (!caster->rayTracingEnabled())
    {
        if (!table.instances.empty())
        {
            throw std::runtime_error("Unavailable tracing unexpectedly produced query instances");
        }
        return;
    }
    if (table.instances.size() != 2 || table.instances[0].ranges.w != 3 || table.instances[1].ranges.w != 4 ||
        table.instances[0].ranges.z != std::numeric_limits<uint32_t>::max() || table.instances[1].ranges.z != 0 ||
        table.instances[1].options.x != 1 || table.triangles.size() != 1 || table.alphaTextures.size() != 1 ||
        table.instances[1].alpha != glm::vec4(2, 3, 0.4f, 0.6f) ||
        table.triangles[0].a != glm::vec4(mesh->vertices[3].texCoord0, mesh->vertices[3].texCoord1))
    {
        throw std::runtime_error("Dense query mapping, firstIndex or albedo alpha/UV metadata differs");
    }
    auto scene = engine.device()->createRayTracingScene("opacity descriptor probe");
    const auto& instance = collection.instances()[1].instance;
    scene->addInstance(instance.mesh, instance.transform, instance.instanceId, instance.mask, gpu::RayTracingOpacity::ForceNonOpaque);
    if (scene->instances().front().opacity != gpu::RayTracingOpacity::ForceNonOpaque)
    {
        throw std::runtime_error("Generic instance opacity was lost");
    }
    caster->material(1).setIsTransparent(true);
    visitor.enqueue(root.get(), engine, queue);
    collection.collect(queue);
    if (collection.instances().size() != 1)
    {
        throw std::runtime_error("Transparent cutout must stay excluded from default query occluders");
    }
}
// Construct before acquisition. Disable tracing here to prove enabled=false
// rendering needs no BLAS; otherwise the two drawables use normal built geometry.
inline std::shared_ptr<scene::Node> shadowProbeScene(Engine& engine, bool tracing = true)
{
    auto root = std::make_shared<scene::Node>("cutout shadow probe");
    for (uint32_t i = 0; i < 2; ++i)
    {
        auto mesh = std::make_shared<Drawable::Mesh>();
        const float left = i == 0 ? -0.95f : 0.05f;
        const float right = i == 0 ? 0.95f : 0.35f;
        const float halfHeight = i == 0 ? 0.95f : 0.3f;
        const float z = i == 0 ? 0.8f : 0.3f;
        const glm::vec3 positions[] = {{left, -halfHeight, z}, {right, -halfHeight, z}, {right, halfHeight, z}, {left, halfHeight, z}};
        const glm::vec2 uv[] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        for (uint32_t v = 0; v < 4; ++v)
        {
            mesh->vertices.push_back({positions[v], {0, 0, -1}, uv[v], uv[v], {1, 0, 0}});
        }
        mesh->indices = {0, 1, 2, 0, 2, 3};
        mesh->submeshes = {{0, 6}};
        auto drawable = std::make_shared<Drawable>();
        drawable->setMesh(mesh);
        drawable->setRayTracingEnabled(tracing);
        drawable->material().setAlbedo(base::Color::White());
        drawable->material().setMetalness(0);
        drawable->material().setRoughness(0.8f);
        drawable->material().setAlphaCutoff(i == 1 ? 0.5f : 0.0f);
        drawable->load(&engine);
        if (i == 1)
        {
            gpu::ImageDescription description;
            description.size = {2, 1};
            description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst;
            auto image = engine.device()->createImage(description);
            const uint8_t pixels[] = {255, 255, 255, 0, 255, 255, 255, 255};
            image->upload({pixels, sizeof(pixels), 8, 0, 0});
            auto settings = std::make_shared<base::Texture>();
            settings->setMinFilter(base::Texture::FilterNearest);
            settings->setMagFilter(base::Texture::FilterNearest);
            auto texture = std::make_shared<Texture>(&engine);
            texture->load(settings, image);
            drawable->drawMaterial(0)->setTexture(TextureSlot::Albedo, texture);
        }
        auto node = std::make_shared<scene::Node>(i == 0 ? "receiver" : "cutout caster");
        node->addComponent(new scene::DrawableComponent(drawable));
        root->addChild(node);
    }
    auto lightNode = std::make_shared<scene::Node>("point light");
    lightNode->addComponent(new scene::TransformComponent(glm::translate(glm::mat4{1}, glm::vec3(-0.6f, 0, -0.5f))));
    auto light = new scene::LightComponent();
    light->light().setType(base::Light::TypeOmni);
    light->light().setIntensity(2);
    light->light().setSourceSize(0);
    light->light().setShadowSamples(1);
    lightNode->addComponent(light);
    root->addChild(lightNode);
    return root;
}
}
