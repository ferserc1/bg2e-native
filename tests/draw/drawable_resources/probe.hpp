#pragma once

#include <bg2e/draw/Drawable.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/BoundingBox.hpp>
#include <bg2e/scene/Node.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

namespace bg2e::draw::tests {

// Call with an initialized Engine while scene/background work is stopped.
// No shader, renderer, UI or active frame is needed.
inline void runDrawableResourceProbe(Engine& engine)
{
    auto mesh = std::make_shared<Drawable::Mesh>();
    mesh->vertices.resize(6);
    const std::array<glm::vec3, 6> positions = {glm::vec3(-1, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0),
        glm::vec3(1, 0, 0), glm::vec3(2, 0, 0), glm::vec3(2, 1, 0)};
    for (size_t i = 0; i < positions.size(); ++i)
    {
        mesh->vertices[i] = {positions[i], {0, 0, 1}, {0, 0}, {1, 1}, {1, 0, 0}};
    }
    mesh->indices = {0, 1, 2, 3, 4, 5};
    mesh->submeshes = {{0, 3}, {3, 3}};
    auto source = std::make_shared<Drawable>();
    source->setMesh(mesh);
    source->setSubmeshName("first", 0);
    source->setSubmeshGroupName("group", 1);
    source->setSubmeshTransform(glm::translate(glm::mat4(1), glm::vec3(5, 0, 0)), 1);
    source->material(0).setRoughnessChannel(2);
    source->material(0).setRoughnessUVSet(1);
    source->material(0).setAlphaCutoff(0.3f);
    source->load(&engine);

    gpu::ImageDescription description;
    description.size = {4, 4};
    description.mipLevels = 3;
    description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst;
    auto image = engine.device()->createImage(description);
    for (uint32_t mip = 0; mip < 3; ++mip)
    {
        const auto extent = image->mipExtent(mip);
        std::vector<uint8_t> pixels(size_t(extent.width) * extent.height * 4, uint8_t(40 + mip * 30));
        image->upload({pixels.data(), pixels.size(), size_t(extent.width) * 4, mip, 0});
    }
    auto texture = std::make_shared<Texture>(&engine);
    auto settings = std::make_shared<base::Texture>();
    texture->load(settings, image);
    source->drawMaterial(0)->setTexture(TextureSlot::Albedo, texture);
    const auto originalRaster = source->drawMesh();
    const auto originalTracing = source->rayTracingMeshes();
    source->setRayTracingEnabled(false); // Already-built geometry must still clone.
    auto copy = source->clone();
    if (copy->mesh() == source->mesh() || copy->drawMesh() == originalRaster ||
        copy->drawMesh()->vertexBuffer() == originalRaster->vertexBuffer() ||
        copy->drawMesh()->indexBuffer() == originalRaster->indexBuffer() ||
        copy->drawMaterial(0) == source->drawMaterial(0) || copy->material(0).roughnessChannel() != 2 ||
        copy->material(0).roughnessUVSet() != 1 || copy->submeshName(0) != "first" || copy->submeshGroupName(1) != "group")
    {
        throw std::runtime_error("Drawable clone shares mutable geometry/material state or lost metadata");
    }
    for (size_t submesh = 0; submesh < source->materials().size(); ++submesh)
    {
        for (size_t slot = 0; slot < 6; ++slot)
        {
            const auto original = source->drawMaterial(uint32_t(submesh))->texture(TextureSlot(slot));
            const auto cloned = copy->drawMaterial(uint32_t(submesh))->texture(TextureSlot(slot));
            if (original == cloned || original->image() == cloned->image() || original->sampler() == cloned->sampler())
            {
                throw std::runtime_error("Drawable clone shares a GPU texture/sampler");
            }
        }
    }
    for (uint32_t mip = 0; mip < 3; ++mip)
    {
        std::vector<uint8_t> pixels;
        copy->drawMaterial(0)->albedoTexture()->image()->readPixels(pixels, {mip, 1, 0, 1});
        if (pixels.empty() || !std::all_of(pixels.begin(), pixels.end(), [mip](uint8_t value)
            {
                return value == uint8_t(40 + mip * 30);
            }))
        {
            throw std::runtime_error("Drawable clone lost authored mip pixels");
        }
    }
    std::vector<uint8_t> changed(4, 200);
    image->upload({changed.data(), changed.size(), 4, 2, 0});
    std::vector<uint8_t> unchanged;
    copy->drawMaterial(0)->albedoTexture()->image()->readPixels(unchanged, {2, 1, 0, 1});
    if (unchanged != std::vector<uint8_t>(4, 100))
    {
        throw std::runtime_error("Editing original texture changed clone pixels");
    }
    source->mesh()->vertices[0].position.x = 99;
    source->material(0).setRoughness(0.1f);
    source->material(0).albedoTexture()->setMinFilter(base::Texture::FilterNearest);
    source->reload();
    if (copy->mesh()->vertices[0].position.x != -1 || copy->drawMesh()->meshData().vertices[0].position.x != -1 ||
        copy->material(0).roughness() == 0.1f || copy->material(0).albedoTexture() == source->material(0).albedoTexture())
    {
        throw std::runtime_error("Editing original changed cloned CPU/material data");
    }
    for (size_t i = 0; i < copy->rayTracingMeshes().size(); ++i)
    {
        const auto cloned = copy->rayTracingMesh(uint32_t(i));
        if (!cloned->isBuilt() || cloned == originalTracing.at(i) ||
            cloned->description().vertexBuffer != copy->drawMesh()->vertexBuffer())
        {
            throw std::runtime_error("Drawable clone did not rebuild independent tracing geometry");
        }
    }
    auto component = std::make_shared<scene::DrawableComponent>(copy);
    bool rejected = false;
    try
    {
        component->drawable();
    }
    catch (const std::logic_error&)
    {
        rejected = true;
    }
    if (!rejected)
    {
        throw std::runtime_error("DrawableComponent accepted a wrong-mode accessor");
    }
    auto node = std::make_shared<scene::Node>();
    node->addComponent(component);
    scene::BoundingBox bounds(node.get());
    if (!bounds.isValid() || bounds.max().x != 7)
    {
        throw std::runtime_error("Draw component bounds lost its submesh transform");
    }
    copy->setSubmeshVisibility(false, 1);
    scene::BoundingBox visibleBounds(node.get());
    if (visibleBounds.max().x != 0)
    {
        throw std::runtime_error("Draw component bounds ignored submesh visibility");
    }
    auto clonedComponent = std::dynamic_pointer_cast<scene::DrawableComponent>(component->clone());
    if (clonedComponent->drawDrawable() == copy || clonedComponent->drawDrawable()->mesh() == copy->mesh())
    {
        throw std::runtime_error("DrawableComponent clone shared the draw connector");
    }
    clonedComponent->clearDrawable();
    component->clearDrawable();
    source->cleanup();
    copy->cleanup();
}

}
