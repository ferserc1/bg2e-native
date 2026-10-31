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

#include <bg2e/draw/pipeline/BloomPass.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <algorithm>
#include <cmath>

namespace bg2e::draw::pipeline {
void BloomSettings::validate() const
{
    if (!std::isfinite(intensity) || intensity < 0 || intensity > 10 ||
        !std::isfinite(threshold) || threshold < 0 || threshold > 65504 ||
        !std::isfinite(scatter) || scatter < 0 || scatter > 1 || levels < 1 || levels > 8 ||
        (resolution != BloomResolution::Half && resolution != BloomResolution::Quarter))
    {
        throw std::invalid_argument("Invalid bloom intensity, threshold, scatter, levels or resolution");
    }
}

void BloomPass::initialize(Engine& engine, const std::filesystem::path& shaderRoot, uint32_t slots)
{
    if (_engine || slots == 0)
    {
        throw std::invalid_argument("Bloom requires a fresh lifecycle and nonzero slot count");
    }
    _engine = &engine;
    auto& device = *engine.device();
    const auto capabilities = device.formatCapabilities(gpu::PixelFormat::R16G16B16A16_SFLOAT);
    if (!capabilities.supports(gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment) ||
        !capabilities.linearFiltering || !capabilities.blend)
    {
        throw std::runtime_error("Bloom requires filterable, blendable RGBA16F attachments");
    }
    gpu::PipelineLayoutDescription layout;
    layout.resourceBindings = {
        {0, {0, 0}, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment},
        {0, {1, 0}, gpu::ResourceType::Sampler, gpu::ShaderStage::Fragment}
    };
    layout.pushConstants.push_back({0, sizeof(Push), gpu::ShaderStage::Fragment});
    _layout = device.createPipelineLayout(layout);
    gpu::ShaderLib library(shaderRoot, engine.backendType());
    _vertex = library.vertex("draw/raster/fullscreen", &device);
    _fragment = library.fragment("draw/raster/bloom", &device);
    gpu::GraphicsPipelineDescription pipeline;
    pipeline.layout = _layout.get();
    pipeline.vertexShader = _vertex.get();
    pipeline.fragmentShader = _fragment.get();
    pipeline.cullMode = gpu::CullMode::None;
    pipeline.depthState = gpu::DepthState{false, false, gpu::CompareOperation::Always};
    pipeline.colorAttachments = {{gpu::PixelFormat::R16G16B16A16_SFLOAT, {}}};
    pipeline.debugName = "bloom downsample";
    _downsample = device.createGraphicsPipeline(pipeline);
    auto& blend = pipeline.colorAttachments[0].blend;
    blend.enabled = true;
    blend.sourceColor = gpu::BlendFactor::SourceAlpha;
    blend.destinationColor = gpu::BlendFactor::OneMinusSourceAlpha;
    pipeline.debugName = "bloom upsample";
    _upsample = device.createGraphicsPipeline(pipeline);
    gpu::SamplerDescription sampler;
    sampler.minFilter = sampler.magFilter = gpu::Filter::Linear;
    sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = gpu::AddressMode::ClampToEdge;
    sampler.maxLod = 0;
    _sampler = device.createSampler(sampler);
    _slots.resize(slots);
}

void BloomPass::draw(const FrameContext& frame, Slot& slot, gpu::Image& source,
    gpu::Image& destination, const Push& push, bool upsample)
{
    if (slot.invocation == slot.bindings.size())
    {
        slot.bindings.push_back(_engine->device()->createResourceSet(_layout.get(), 0));
    }
    const auto& resources = slot.bindings.at(slot.invocation++);
    resources->setSampledImage({0, 0}, &source);
    resources->setSampler({1, 0}, _sampler.get());
    resources->update();
    auto& command = frame.commandBuffer;
    command.transition(&source, gpu::ImageLayout::ShaderReadOnly);
    command.transition(&destination, gpu::ImageLayout::ColorAttachment);
    gpu::RenderingDescription rendering;
    rendering.colors.push_back({.image = &destination,
        .load = upsample ? gpu::AttachmentLoadOperation::Load : gpu::AttachmentLoadOperation::DontCare});
    command.beginRendering(rendering);
    auto* pipeline = upsample ? _upsample.get() : _downsample.get();
    command.bindPipeline(pipeline);
    command.bindResourceSet(pipeline, 0, resources.get());
    command.pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(Push), &push);
    command.draw(3);
    command.endRendering();
    command.transition(&destination, gpu::ImageLayout::ShaderReadOnly);
}

gpu::Image* BloomPass::record(const FrameContext& frame, gpu::Image& hdr, const BloomSettings& settings)
{
    settings.validate();
    if (&frame.engine != _engine || frame.frameSlot >= _slots.size() ||
        hdr.device() != _engine->device() || hdr.pixelFormat() != gpu::PixelFormat::R16G16B16A16_SFLOAT)
    {
        throw std::invalid_argument("Bloom frame slot or HDR input mismatch");
    }
    auto& slot = _slots.at(frame.frameSlot);
    slot.invocation = 0; // The caller has acquired and retired this slot.
    if (!settings.enabled || settings.intensity == 0)
    {
        slot.bindings.clear();
        slot.images.clear();
        slot.sourceExtent = {};
        return nullptr;
    }
    gpu::GpuTimingScope timing(frame.commandBuffer, "post.bloom");
    const auto divisor = uint32_t(settings.resolution);
    if (slot.sourceExtent != hdr.size() || slot.divisor != divisor || slot.levels != settings.levels)
    {
        std::vector<std::shared_ptr<gpu::Image>> images;
        gpu::Size2D extent{std::max(1u, (hdr.width() + divisor - 1) / divisor),
            std::max(1u, (hdr.height() + divisor - 1) / divisor)};
        for (uint32_t i = 0; i < settings.levels; ++i)
        {
            gpu::ImageDescription description;
            description.size = extent;
            description.format = gpu::PixelFormat::R16G16B16A16_SFLOAT;
            description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::ColorAttachment;
            description.debugName = "bloom pyramid " + std::to_string(i);
            images.push_back(_engine->device()->createImage(description));
            if (extent.width == 1 && extent.height == 1)
            {
                break;
            }
            extent = {std::max(1u, (extent.width + 1) / 2), std::max(1u, (extent.height + 1) / 2)};
        }
        slot.bindings.clear();
        slot.images = std::move(images);
        slot.sourceExtent = hdr.size();
        slot.divisor = divisor;
        slot.levels = settings.levels;
    }
    gpu::Image* source = &hdr;
    Push push;
    push.parameters = {settings.threshold, settings.scatter, 0, 0};
    for (size_t i = 0; i < slot.images.size(); ++i)
    {
        auto& destination = *slot.images[i];
        push.extent = {float(destination.width()), float(destination.height()), float(divisor), i == 0 ? 0.0f : 1.0f};
        draw(frame, slot, *source, destination, push, false);
        source = &destination;
    }
    for (size_t i = slot.images.size() - 1; i > 0; --i)
    {
        auto& destination = *slot.images[i - 1];
        push.extent = {float(destination.width()), float(destination.height()), 0, 2};
        draw(frame, slot, *slot.images[i], destination, push, true);
    }
    return slot.images.front().get();
}

void BloomPass::cleanup()
{
    _slots.clear();
    _upsample.reset();
    _downsample.reset();
    _sampler.reset();
    _vertex.reset();
    _fragment.reset();
    _layout.reset();
    _engine = nullptr;
}
}
