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

#include <bg2e/draw/pipeline/FullscreenPass.hpp>
#include <bg2e/draw/pipeline/TemporaryResourcePool.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <limits>

namespace bg2e::draw::pipeline {
void FullscreenPass::initialize(Engine& engine, const std::filesystem::path& shaderRoot,
    const std::string& fragment, std::shared_ptr<gpu::PipelineLayout> layout,
    gpu::PixelFormat output, uint32_t slots)
{
    if (_engine || !layout || layout->device() != engine.device() || slots == 0)
    {
        throw std::invalid_argument("Fullscreen pass requires a fresh lifecycle, matching layout and slots");
    }
    _engine = &engine;
    _layout = std::move(layout);
    gpu::ShaderLib library(shaderRoot, engine.backendType());
    _vertex = library.vertex("draw/raster/fullscreen", engine.device());
    _fragment = library.fragment(fragment, engine.device());
    gpu::GraphicsPipelineDescription description;
    description.layout = _layout.get();
    description.vertexShader = _vertex.get();
    description.fragmentShader = _fragment.get();
    description.colorFormat = output;
    description.cullMode = gpu::CullMode::None;
    description.depthState = gpu::DepthState{false, false, gpu::CompareOperation::Always};
    _pipeline = engine.device()->createGraphicsPipeline(description);
    gpu::SamplerDescription sampler;
    // Nearest supports every sampled G-buffer format including depth. Final
    // reconstruction implements bilinear explicitly, independent of format support.
    sampler.minFilter = sampler.magFilter = gpu::Filter::Nearest;
    sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = gpu::AddressMode::ClampToEdge;
    sampler.maxLod = 0;
    _sampler = engine.device()->createSampler(sampler);
    _sets.resize(slots);
    _serial.assign(slots, std::numeric_limits<uint64_t>::max());
}

void FullscreenPass::beginFrame(const FrameContext& frame)
{
    if (&frame.engine != _engine || frame.frameSlot >= _sets.size())
    {
        throw std::invalid_argument("Fullscreen frame slot/context mismatch");
    }
    if (_serial[frame.frameSlot] != frame.frameNumber)
    {
        _sets[frame.frameSlot].clear();
        if (_pool)
        {
            _pool->collect();
        }
        _serial[frame.frameSlot] = frame.frameNumber;
    }
}

void FullscreenPass::draw(const FrameContext& frame, gpu::Image& output,
    const std::vector<gpu::Image*>& inputs, uint32_t imageSet,
    const std::vector<std::pair<uint32_t, gpu::ResourceSet*>>& sets, const void* push, uint32_t pushSize, const std::vector<uint32_t>& metalImages, const std::vector<uint32_t>& metalSamplers, std::optional<gpu::ScissorRegion> region)
{
    beginFrame(frame);
    if ((!metalImages.empty() && metalImages.size() != inputs.size()) ||
        (!metalSamplers.empty() && metalSamplers.size() != inputs.size()))
    {
        throw std::invalid_argument("Fullscreen native image map must match input count");
    }
    auto& command = frame.commandBuffer;
    if (!_pool)
    {
        _pool = std::make_shared<TemporaryResourcePool>(*_engine->device());
    }
    _pool->setEnabled(_poolingEnabled);
    auto pooled = _pool->bindings(command, _layout, imageSet, 0, {frame.frameSlot, _sets[frame.frameSlot].size(), 0, 0});
    const auto& resources = pooled->resources();
    for (uint32_t i = 0; i < inputs.size(); ++i)
    {
        if (!inputs[i] || inputs[i] == &output || inputs[i]->device() != _engine->device())
        {
            throw std::invalid_argument("Fullscreen inputs must be valid, distinct from output and on the same device");
        }
        command.transition(inputs[i], gpu::ImageLayout::ShaderReadOnly);
        const auto metal = metalImages.empty() ? i : metalImages[i];
        pooled->setSampledImage({i, metal}, inputs[i]);
        pooled->setSampler({uint32_t(inputs.size()) + i, metalSamplers.empty() ? metal : metalSamplers[i]}, _sampler.get());
    }
    pooled->update();
    _sets[frame.frameSlot].push_back(pooled);
    command.transition(&output, gpu::ImageLayout::ColorAttachment);
    gpu::RenderingDescription rendering;
    rendering.colors.push_back({.image = &output, .load = region ? gpu::AttachmentLoadOperation::Load : gpu::AttachmentLoadOperation::DontCare});
    rendering.scissor = region;
    command.beginRendering(rendering);
    command.bindPipeline(_pipeline.get());
    command.bindResourceSet(_pipeline.get(), imageSet, resources.get());
    for (const auto& [index, set] : sets)
    {
        command.bindResourceSet(_pipeline.get(), index, set);
    }
    if (pushSize)
    {
        command.pushConstants(gpu::ShaderStage::Fragment, 0, pushSize, push);
    }
    command.draw(3);
    command.endRendering();
    command.transition(&output, gpu::ImageLayout::ShaderReadOnly);
}

void FullscreenPass::cleanup()
{
    if (_engine)
    {
        _engine->device()->waitIdle();
    }
    _sets.clear();
    _pool.reset();
    _serial.clear();
    _pipeline.reset();
    _sampler.reset();
    _vertex.reset();
    _fragment.reset();
    _layout.reset();
    _engine = nullptr;
}

TemporaryPoolStatistics FullscreenPass::poolStatistics() const
{
    return _pool ? _pool->statistics() : TemporaryPoolStatistics{};
}

}
