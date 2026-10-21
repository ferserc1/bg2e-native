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

#include "retirement_numeric.hpp"
#include <bg2e/draw/pipeline/GBuffer.hpp>
#include <cstring>

namespace bg2e::bake::tests {
class SolidRgbLightmapGenerator final : public bg2e::base::ProceduralTextureGenerator {
  public:
    SolidRgbLightmapGenerator()
        : ProceduralTextureGenerator(2, 2, 4)
    {
    }

    uint8_t* generate() override
    {
        auto* pixels = new uint8_t[width() * height() * 4];
        for (uint32_t pixel = 0; pixel < width() * height(); ++pixel)
        {
            pixels[pixel * 4 + 0] = 230;
            pixels[pixel * 4 + 1] = 82;
            pixels[pixel * 4 + 2] = 31;
            pixels[pixel * 4 + 3] = 255;
        }
        return pixels;
    }

    std::string imageIdentifier() override
    {
        return "lightmap-fixture-rgb-230-82-31";
    }
    bg2e::base::Color::Type colorType() override
    {
        return bg2e::base::Color::TypeLinear;
    }
};

inline void runRetirementMaterialProbe(draw::Engine& engine, const std::filesystem::path& shaders)
{
    auto drawable = std::make_shared<draw::Drawable>();
    drawable->setMesh(makeTriangleMesh());
    drawable->load(&engine);
    auto material = drawable->drawMaterial(0);
    if (draw::packMaterial(*material).uvSets[1].z & draw::MaterialBakedMultiplier)
    {
        throw std::runtime_error("An ordinary AO material was classified as a baked multiplier");
    }
    auto texture = std::make_shared<base::Texture>();
    texture->setProceduralGenerator(new SolidRgbLightmapGenerator());
    auto& attributes = drawable->material();
    attributes.setAoTexture(texture);
    attributes.setAoUVSet(1);
    attributes.setIndirectTextureMode(base::MaterialAttributes::IndirectTextureMode::BakedGlobalIllumination);
    material->updateTextures();
    const auto flags = draw::packMaterial(*material).uvSets[1].z;
    if (!(flags & draw::MaterialBakedMultiplier) || !(flags & draw::MaterialBakedGI))
    {
        throw std::runtime_error("Explicit linear RGB UV2 baked material lost its packed flags");
    }
    auto target = std::make_shared<scene::Node>("Baked RGB fixture");
    target->addComponent(std::make_shared<scene::DrawableComponent>(drawable));
    draw::RenderQueue queue;
    queue.beginFrame(engine);
    queue.enqueue(*target, drawable, 0, glm::mat4(1));
    queue.sort({0, 0, 1});
    draw::pipeline::GBuffer pass(shaders);
    draw::pipeline::LayerConfiguration configuration;
    configuration.outputExtent = configuration.internalExtent = {64, 64};
    configuration.contract = pass.contract();
    if (configuration.contract.outputs.size() != 7 ||
        configuration.contract.outputs[5].semantic != "bakedMultiplier" ||
        configuration.contract.outputs[5].format != gpu::PixelFormat::R16G16B16A16_SFLOAT ||
        configuration.contract.outputs[6].format != gpu::PixelFormat::D32_SFLOAT)
    {
        throw std::runtime_error("Camera G-buffer lost six color attachments and D32 depth");
    }
    std::vector<std::shared_ptr<gpu::Image>> images;
    std::vector<draw::pipeline::ImageBinding> outputs;
    for (const auto& slot : configuration.contract.outputs)
    {
        gpu::ImageDescription description;
        description.size = {64, 64};
        description.format = slot.format;
        description.usage = slot.usage | gpu::ImageUsage::TransferSrc;
        images.push_back(engine.device()->createImage(description));
        outputs.push_back({slot.semantic, images.back().get()});
    }
    pass.initialize(engine, configuration);
    auto layout = engine.device()->createPipelineLayout(draw::drawBindingLayout());
    draw::FrameDataBinding frameBinding;
    draw::ObjectDataBinding objectBinding;
    frameBinding.initialize(engine, layout, 1);
    objectBinding.initialize(engine, layout, 1);
    engine.device()->immediateSubmit([&](gpu::CommandBuffer* command)
    {
        draw::FrameContext frame{engine, *command, *images[0], {64, 64}, 1, 0};
        frameBinding.update(frame, glm::mat4(1), glm::mat4(1));
        objectBinding.prepare(frame, queue);
        draw::pipeline::RasterFrame raster;
        raster.queue = &queue;
        raster.frame = &frameBinding;
        raster.object = &objectBinding;
        const std::vector<draw::pipeline::ImageBinding> inputs;
        pass.record({frame, configuration, inputs, outputs, target.get(), &raster});
    });
    std::vector<uint8_t> baked, packed;
    images[5]->readPixels(baked, images[5]->fullRange());
    images[2]->readPixels(packed, images[2]->fullRange());
    if (baked.size() != 64 * 64 * 8 || packed.size() != 64 * 64 * 4)
    {
        throw std::runtime_error("G-buffer RGB/packed material readback has incorrect byte dimensions");
    }
    const auto half = [](const uint8_t* bytes)
    {
        uint16_t value;
        std::memcpy(&value, bytes, sizeof(value));
        // This fixture expects positive normalized UNORM inputs stored as half.
        return std::ldexp(float(1024 + (value & 1023)), int((value >> 10) & 31) - 25);
    };
    bool found = false;
    for (size_t i = 0; i < 64 * 64; ++i)
    {
        const auto* p = baked.data() + i * 8;
        if (std::abs(half(p) - 230.f / 255) < .005f &&
            std::abs(half(p + 2) - 82.f / 255) < .005f &&
            std::abs(half(p + 4) - 31.f / 255) < .005f)
        {
            found = true;
            if (packed[i * 4 + 2] != 255)
            {
                throw std::runtime_error("Baked RGB map was also applied as scalar material AO");
            }
        }
    }
    if (!found)
    {
        throw std::runtime_error("Camera G-buffer collapsed or lost baked RGB channels");
    }
    objectBinding.cleanup();
    frameBinding.cleanup();
    pass.cleanup();
    queue.clear();
    drawable->cleanup();
}
}
