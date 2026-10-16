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

#include <bg2e/draw/pipeline/OverlayPass.hpp>
#include <bg2e/draw/Engine.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <cmath>
#include <utility>

namespace bg2e::draw::pipeline {

MeshOverlayPass::MeshOverlayPass(MeshOverlaySettings settings) : _settings(std::move(settings))
{
}

const MeshOverlaySettings& MeshOverlayPass::settings() const
{
    return _settings;
}

void MeshOverlayPass::initialize(Engine& engine, const OverlayConfiguration& configuration)
{
    if (_engine || !engine.device() || !engine.device()->isValid() || configuration.extent.isZero() || !configuration.frameSlots ||
        (configuration.depthFormat != gpu::PixelFormat::D16_UNORM && configuration.depthFormat != gpu::PixelFormat::D32_SFLOAT))
    {
        throw std::invalid_argument("Overlay initialization requires fresh state, extent, slots and depth-only format");
    }
    if (!std::isfinite(_settings.highlightDepthBias) || _settings.highlightDepthBias < 0 || _settings.highlightDepthBias > 1)
    {
        throw std::invalid_argument("Overlay highlight depth bias must be finite in [0,1]");
    }
    _engine = &engine;
    _configuration = configuration;
    gpu::PipelineLayoutDescription layout;
    layout.pushConstants.push_back({0, 80, gpu::ShaderStage::Vertex});
    _layout = engine.device()->createPipelineLayout(layout);
    gpu::ShaderLib shaders(_settings.shaderRoot.empty() ? base::PlatformTools::shaderPath() : _settings.shaderRoot, engine.backendType());
    auto vertex = shaders.vertex(_settings.gizmoShader, engine.device());
    auto fragment = shaders.fragment(_settings.gizmoShader, engine.device());
    auto highlightVertex = shaders.vertex(_settings.highlightShader, engine.device());
    auto highlightFragment = shaders.fragment(_settings.highlightShader, engine.device());
    _shaders = {vertex, fragment, highlightVertex, highlightFragment};
    const auto make = [&](bool lines, bool test, bool write, bool blend, bool mask, bool highlight)
    {
        gpu::GraphicsPipelineDescription description;
        description.vertexShader = highlight ? highlightVertex.get() : vertex.get();
        description.fragmentShader = highlight ? highlightFragment.get() : fragment.get();
        description.layout = _layout.get();
        description.topology = lines ? gpu::PrimitiveTopology::LineList : gpu::PrimitiveTopology::TriangleList;
        description.depthFormat = configuration.depthFormat;
        description.cullMode = gpu::CullMode::None;
        description.depthState = gpu::DepthState{test, write, gpu::CompareOperation::LessOrEqual};
        gpu::BlendState blending;
        blending.enabled = blend;
        blending.sourceColor = gpu::BlendFactor::SourceAlpha;
        blending.destinationColor = gpu::BlendFactor::OneMinusSourceAlpha;
        blending.sourceAlpha = gpu::BlendFactor::One;
        blending.destinationAlpha = gpu::BlendFactor::OneMinusSourceAlpha;
        blending.writeMask = mask ? gpu::ColorWriteMask::None : gpu::ColorWriteMask::All;
        description.colorAttachments.push_back({configuration.colorFormat, blending});
        description.addVertexBufferDescription(Drawable::DrawMesh::vertexBufferDescription());
        return engine.device()->createGraphicsPipeline(description);
    };
    _depth = make(false, true, true, false, true, false);
    _highlight = make(_settings.highlightLines, _settings.highlightDepthTest, false, _settings.highlightBlend, false, true);
    _type = make(_settings.typeGizmoLines, _settings.typeGizmoDepthTest, false, _settings.typeGizmoBlend, false, false);
    _transform = make(false, true, true, _settings.transformBlend, false, false);
    buildDepth();
}

void MeshOverlayPass::buildDepth()
{
    std::vector<Slot> next(_configuration.frameSlots);
    gpu::ImageDescription description;
    description.size = _configuration.extent;
    description.format = _configuration.depthFormat;
    description.usage = gpu::ImageUsage::DepthStencil;
    for (auto& slot : next)
    {
        slot.sceneDepth = _engine->device()->createImage(description);
        slot.transformDepth = _engine->device()->createImage(description);
    }
    _slots = std::move(next);
}

void MeshOverlayPass::resize(const OverlayConfiguration& configuration)
{
    if (!_engine || configuration.extent.isZero() || configuration.colorFormat != _configuration.colorFormat ||
        configuration.depthFormat != _configuration.depthFormat || configuration.frameSlots != _configuration.frameSlots)
    {
        throw std::invalid_argument("Overlay resize requires initialized matching formats/slots");
    }
    _engine->device()->waitIdle();
    _configuration = configuration;
    buildDepth();
}

void MeshOverlayPass::validateDraw(const OverlayDraw& draw) const
{
    if (!draw.mesh || draw.submesh >= draw.mesh->submeshCount() ||
        !draw.mesh->vertexBuffer() || !draw.mesh->indexBuffer() ||
        !draw.mesh->vertexBuffer()->isValid() || !draw.mesh->indexBuffer()->isValid() ||
        draw.mesh->vertexBuffer()->device() != _engine->device() || draw.mesh->indexBuffer()->device() != _engine->device())
    {
        throw std::invalid_argument("Overlay mesh must be live and belong to its device");
    }
    for (int i = 0; i < 4; ++i)
    {
        if (!std::isfinite(draw.color[i]))
        {
            throw std::invalid_argument("Overlay color must be finite");
        }
        for (int j = 0; j < 4; ++j)
        {
            if (!std::isfinite(draw.world[i][j]))
            {
                throw std::invalid_argument("Overlay transform must be finite");
            }
        }
    }
}

void MeshOverlayPass::record(const OverlayContext& context, const OverlayDrawList& draws)
{
    const auto& frame = context.frame;
    if (!_engine || &frame.engine != _engine || frame.frameSlot >= _slots.size() ||
        frame.extent != _configuration.extent || frame.commandBuffer.hasActiveScope() ||
        frame.colorTarget.device() != _engine->device() || !frame.colorTarget.isValid() ||
        frame.colorTarget.size() != frame.extent || frame.colorTarget.pixelFormat() != _configuration.colorFormat ||
        frame.colorTarget.currentLayout() == gpu::ImageLayout::Undefined ||
        !gpu::hasFlag(frame.colorTarget.usage(), gpu::ImageUsage::ColorAttachment) ||
        !gpu::hasFlag(frame.colorTarget.usage(), gpu::ImageUsage::Sampled))
    {
        throw std::invalid_argument("Overlay needs acquired output-sized final color outside a command scope");
    }
    for (const auto* list : {&draws.occluders, &draws.highlights, &draws.typeGizmos, &draws.transformGizmos})
    {
        for (const auto& draw : *list)
        {
            validateDraw(draw);
        }
    }
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            if (!std::isfinite(context.view[i][j]) || !std::isfinite(context.projection[i][j]))
            {
                throw std::invalid_argument("Overlay camera must be finite and unjittered");
            }
        }
    }
    auto& slot = _slots[frame.frameSlot];
    slot.draws = draws; // Retain all GPU geometry until this acquired slot completes.
    if (draws.highlights.empty() && draws.typeGizmos.empty() && draws.transformGizmos.empty())
    {
        frame.commandBuffer.transition(&frame.colorTarget, gpu::ImageLayout::ShaderReadOnly);
        return;
    }
    auto& command = frame.commandBuffer;
    struct Push {
        glm::mat4 mvp;
        glm::vec4 color;
    };
    static_assert(sizeof(Push) == 80);
    const auto render = [&](gpu::GraphicsPipeline* pipeline, const std::vector<OverlayDraw>& list, float bias = 0.0f)
    {
        command.bindPipeline(pipeline);
        for (const auto& draw : list)
        {
            Push push{context.projection * context.view * draw.world, draw.color};
            for (int column = 0; column < 4; ++column)
            {
                push.mvp[column][2] -= bias * push.mvp[column][3];
            }
            command.pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(push), &push);
            command.bindVertexBuffer(0, draw.mesh->vertexBuffer());
            command.bindIndexBuffer(draw.mesh->indexBuffer());
            draw.mesh->drawSubmesh(&command, draw.submesh);
        }
    };
    const auto begin = [&](gpu::Image* depth)
    {
        // ShaderReadOnly -> attachment provides a dependency from prior color writes,
        // including another overlay. Do not mutate any renderer-owned scene depth.
        command.transition(&frame.colorTarget, gpu::ImageLayout::ShaderReadOnly);
        command.transition(&frame.colorTarget, gpu::ImageLayout::ColorAttachment);
        command.transition(depth, gpu::ImageLayout::DepthAttachment);
        gpu::RenderingDescription rendering;
        rendering.colors.push_back({.image = &frame.colorTarget, .load = gpu::AttachmentLoadOperation::Load});
        rendering.depth = gpu::DepthAttachmentDescription{.image = depth, .load = gpu::AttachmentLoadOperation::Clear};
        command.beginRendering(rendering);
    };
    if (!draws.highlights.empty() || !draws.typeGizmos.empty())
    {
        begin(slot.sceneDepth.get());
        if (_settings.highlightDepthTest || _settings.typeGizmoDepthTest)
        {
            render(_depth.get(), draws.occluders);
        }
        render(_highlight.get(), draws.highlights, _settings.highlightDepthTest ? _settings.highlightDepthBias : 0.0f);
        render(_type.get(), draws.typeGizmos);
        command.endRendering();
    }
    if (!draws.transformGizmos.empty())
    {
        begin(slot.transformDepth.get());
        render(_transform.get(), draws.transformGizmos);
        command.endRendering();
    }
    command.transition(&frame.colorTarget, gpu::ImageLayout::ShaderReadOnly);
}

void MeshOverlayPass::cleanup()
{
    if (_engine)
    {
        _engine->device()->waitIdle();
    }
    _slots.clear();
    _transform.reset();
    _type.reset();
    _highlight.reset();
    _depth.reset();
    _shaders.clear();
    _layout.reset();
    _configuration = {};
    _engine = nullptr;
}

}
