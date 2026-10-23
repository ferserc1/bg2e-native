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

#include <bg2e/draw/all.hpp>
#include <bg2e/scene/Node.hpp>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace bg2e::draw::tests {
// Application-owned compatible subclass, retained outside the renderer.
class CountingGBuffer : public pipeline::GBuffer {
  public:
    using GBuffer::GBuffer;
    uint32_t opaquePasses = 0;
    uint32_t transparentPasses = 0;
    std::shared_ptr<gpu::Image> receiverDepth;
    void record(const pipeline::PassContext& context) override
    {
        GBuffer::record(context);
        if (context.raster && (context.raster->item || !context.raster->transparentItems.empty()))
        {
            ++transparentPasses;
            if (context.raster->captureReceiverDepth)
            {
                receiverDepth = std::static_pointer_cast<gpu::Image>(context.outputs.at(6).image->shared_from_this());
            }
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
inline std::shared_ptr<scene::Node> deferredProbeScene(Engine& engine, bool overlappingGlass = false)
{
    auto root = std::make_shared<scene::Node>("deferred probe");
    const base::Color colors[] = {{1, 0, 0, 1}, {0, 1, 0, 0.5f}, {0, 0, 1, 0.5f}};
    const float depths[] = {0.5f, overlappingGlass ? 0.35f : 0.8f, 0.2f};
    for (uint32_t i = 0; i < 3; ++i)
    {
        auto mesh = std::make_shared<Drawable::Mesh>();
        const glm::vec3 positions[] = {{-0.7f, -0.7f, 0}, {0.7f, -0.7f, 0}, {0.7f, 0.7f, 0}, {-0.7f, 0.7f, 0}};
        for (const auto& position : positions)
        {
            const auto vertexPosition = overlappingGlass && i == 2
                ? glm::vec3(position.x*0.5f,position.y,position.z) : position;
            mesh->vertices.push_back({vertexPosition, {0, 0, 1}, {0.5f, 0.5f}, {0.5f, 0.5f}, {1, 0, 0}});
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
// Call only after the probe frame has completed. A sampled opaque depth of
// 0.5 (or background 1.0) cannot reconstruct the visible glass at depth 0.2.
inline void checkDeferredReceiverDepth(const CountingGBuffer& pass)
{
    if (!pass.receiverDepth || pass.receiverDepth->pixelFormat() != gpu::PixelFormat::D32_SFLOAT)
    {
        throw std::runtime_error("Deferred receiver probe requires captured D32 scratch depth");
    }
    std::vector<uint8_t> bytes;
    pass.receiverDepth->readPixels(bytes,{},gpu::ImageLayout::ShaderReadOnly);
    const auto extent = pass.receiverDepth->size();
    const size_t center = (size_t(extent.height/2)*extent.width + extent.width/2)*sizeof(float);
    if (bytes.size() != size_t(extent.width)*extent.height*sizeof(float))
    {
        throw std::runtime_error("Deferred receiver depth readback extent differs");
    }
    float depth = 0;
    std::memcpy(&depth,bytes.data()+center,sizeof(depth));
    if (!std::isfinite(depth) || std::abs(depth-0.2f) > 1e-5f)
    {
        throw std::runtime_error("Transparent lighting depth describes the opaque background instead of the receiver");
    }
}

inline void checkDeferredCenter(const std::vector<uint8_t>& rgba, gpu::Size2D extent, bool overlappingGlass = false)
{
    if (extent.isZero() || rgba.size() != size_t(extent.width) * extent.height * 4)
    {
        throw std::invalid_argument("Expected tightly packed RGBA8 readback");
    }
    const size_t center = (size_t(extent.height / 2) * extent.width + extent.width / 2) * 4;
    const float expected = std::pow(1.0f - std::exp(-0.5f), 1.0f / 2.2f) * 255.0f;
    const float quarter = std::pow(1.0f - std::exp(-0.25f), 1.0f / 2.2f) * 255.0f;
    if (std::abs(float(rgba[center]) - (overlappingGlass ? quarter : expected)) > 5 ||
        std::abs(float(rgba[center + 1]) - (overlappingGlass ? quarter : 0.0f)) > 5 ||
        std::abs(float(rgba[center + 2]) - expected) > 5 || rgba[center + 3] != 255)
    {
        throw std::runtime_error("Deferred alpha/depth composition or final color processing differs");
    }
    if (overlappingGlass)
    {
        // Outside the last blue draw, the earlier green glass must survive.
        const size_t left = (size_t(extent.height/2)*extent.width + extent.width/4)*4;
        if (std::abs(float(rgba[left])-expected) > 5 || std::abs(float(rgba[left+1])-expected) > 5 ||
            rgba[left+2] > 3 || rgba[left+3] != 255)
        {
            throw std::runtime_error("Transparent composition lost an earlier nonoverlapping surface");
        }
    }
}
}
