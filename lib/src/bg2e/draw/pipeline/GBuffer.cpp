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

#include <bg2e/draw/pipeline/GBuffer.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/gpu/ShaderLib.hpp>

namespace bg2e::draw::pipeline {
std::vector<gpu::PixelFormat> gbufferFormats()
{
    using F = gpu::PixelFormat;
    return {F::R8G8B8A8_UNORM, F::R16G16B16A16_SFLOAT, F::R8G8B8A8_UNORM,
        F::R8G8B8A8_UNORM, F::R8G8B8A8_UNORM, F::R16G16B16A16_SFLOAT};
}

PassContract gbufferContract(gpu::PixelFormat depth)
{
    PassContract result{"draw.raster.gbuffer.v1", {}, {}};
    const std::vector<std::string> names{"albedo", "normalEmission", "material", "fresnelFlags", "sheenRefraction", "bakedMultiplier"};
    const auto formats = gbufferFormats();
    for (size_t i = 0; i < names.size(); ++i)
    {
        result.outputs.push_back({names[i], formats[i], ExtentDomain::Internal, gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::Sampled});
    }
    result.outputs.push_back({"depth", depth, ExtentDomain::Internal, gpu::ImageUsage::DepthStencil | gpu::ImageUsage::Sampled});
    return result;
}

GBuffer::GBuffer(std::filesystem::path shaderRoot, gpu::PixelFormat depth) : _shaderRoot(shaderRoot.empty() ? base::PlatformTools::shaderPath() : std::move(shaderRoot)), _contract(gbufferContract(depth))
{
}

const PassContract& GBuffer::contract() const
{
    return _contract;
}

void GBuffer::initialize(Engine& engine, const LayerConfiguration& configuration)
{
    if (_engine)
    {
        throw std::logic_error("Clean up GBuffer before initialization");
    }
    _engine = &engine;
    _contract = gbufferContract(configuration.contract.outputs.at(6).format);
    _layout = engine.device()->createPipelineLayout(drawBindingLayout());
    gpu::ShaderLib library(_shaderRoot, engine.backendType());
    _vertex = library.vertex("draw/raster/gbuffer", engine.device());
    _fragment = library.fragment("draw/raster/gbuffer", engine.device());
    gpu::GraphicsPipelineDescription description;
    description.vertexShader = _vertex.get();
    description.fragmentShader = _fragment.get();
    description.layout = _layout.get();
    description.depthFormat = _contract.outputs.back().format;
    description.cullMode = gpu::CullMode::None;
    description.depthState = gpu::DepthState{true, true, gpu::CompareOperation::LessOrEqual};
    for (auto format : gbufferFormats())
    {
        description.colorAttachments.push_back({format, {}});
    }
    description.addVertexBufferDescription(Drawable::DrawMesh::vertexBufferDescription());
    _pipeline = engine.device()->createGraphicsPipeline(description);
}

void GBuffer::attachScene(std::shared_ptr<scene::Node>)
{
}

void GBuffer::resize(const LayerConfiguration&)
{
}

void GBuffer::update(const FrameContext&)
{
}

void GBuffer::record(const PassContext& context)
{
    const auto* raster = context.raster;
    if (!_pipeline || &context.frame.engine != _engine || !raster || !raster->queue || !raster->frame || !raster->object)
    {
        throw std::invalid_argument("GBuffer needs the current draw queue and bindings");
    }
    raster->queue->validate(context.frame.engine);
    auto& command = context.frame.commandBuffer;
    gpu::RenderingDescription rendering;
    for (size_t i = 0; i < 6; ++i)
    {
        auto* image = context.outputs.at(i).image;
        command.transition(image, gpu::ImageLayout::ColorAttachment);
        gpu::ColorAttachmentDescription attachment;
        attachment.image = image;
        attachment.load = gpu::AttachmentLoadOperation::Clear;
        attachment.clear.floatingPoint = i == 5 ? gpu::Color{1, 1, 1, 1} : gpu::Color{0, 0, 0, 0};
        rendering.colors.push_back(attachment);
    }
    auto* depth = context.outputs.at(6).image;
    command.transition(depth, gpu::ImageLayout::DepthAttachment);
    rendering.depth = gpu::DepthAttachmentDescription{.image = depth,
        .load = raster->item ? gpu::AttachmentLoadOperation::Load : gpu::AttachmentLoadOperation::Clear};
    command.beginRendering(rendering);
    command.bindPipeline(_pipeline.get());
    command.bindResourceSet(_pipeline.get(), 0, raster->frame->set(context.frame));
    const auto draw = [&](const RenderQueue::Item& item)
    {
        command.bindResourceSet(_pipeline.get(), 1, raster->object->set(context.frame, item.objectId));
        command.bindVertexBuffer(0, item.drawMesh->vertexBuffer());
        command.bindIndexBuffer(item.drawMesh->indexBuffer());
        item.drawMesh->drawSubmesh(&command, item.submeshIndex);
    };
    if (raster->item)
    {
        draw(*raster->item);
    }
    else
    {
        for (const auto& item : raster->queue->queue(RenderQueueType::Opaque))
        {
            draw(item);
        }
    }
    command.endRendering();
    for (const auto& output : context.outputs)
    {
        command.transition(output.image, gpu::ImageLayout::ShaderReadOnly);
    }
}

void GBuffer::cleanup()
{
    if (_engine)
    {
        _engine->device()->waitIdle();
    }
    _pipeline.reset();
    _layout.reset();
    _vertex.reset();
    _fragment.reset();
    _engine = nullptr;
}
}
