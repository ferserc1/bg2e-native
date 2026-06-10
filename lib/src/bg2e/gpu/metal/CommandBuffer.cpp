/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/gpu/metal/CommandBuffer.hpp>
#include <bg2e/gpu/metal/GraphicsPipeline.hpp>
#include <bg2e/gpu/metal/Image.hpp>
#include <bg2e/gpu/metal/SurfaceFrame.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

CommandBuffer::CommandBuffer(metal::Device* device, MTL::CommandBuffer* cmd)
    : _device(device), _cmd(cmd)
{
    if (_cmd)
    {
        _cmd->retain();
    }
}

CommandBuffer::~CommandBuffer()
{
    if (_encoder)
    {
        _encoder->release();
        _encoder = nullptr;
    }
    if (_passDesc)
    {
        _passDesc->release();
        _passDesc = nullptr;
    }
    if (_cmd)
    {
        _cmd->release();
        _cmd = nullptr;
    }
}

void CommandBuffer::begin()
{
    _recording = true;
}

void CommandBuffer::end()
{
    _recording = false;
}

void CommandBuffer::transition(gpu::Image* image, ImageLayout newLayout)
{
    // Metal manages hazards automatically for render targets.
    // This is bookkeeping only for validation/debug.
    image->setCurrentLayout(newLayout);
}

void CommandBuffer::beginRendering(gpu::SurfaceFrame* frame)
{
    _renderFrame = dynamic_cast<metal::SurfaceFrame*>(frame);
    if (!_renderFrame)
    {
        throw std::runtime_error("metal::CommandBuffer::beginRendering: not a metal::SurfaceFrame");
    }

    _passDesc = MTL::RenderPassDescriptor::alloc()->init();

    auto* colorImg = dynamic_cast<metal::Image*>(_renderFrame->colorImage());
    if (colorImg && colorImg->texture())
    {
        _passDesc->colorAttachments()->object(0)->setTexture(colorImg->texture());
        _passDesc->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionDontCare);
        _passDesc->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
    }

    auto* depthImg = dynamic_cast<metal::Image*>(_renderFrame->depthImage());
    if (depthImg && depthImg->isValid() && depthImg->texture())
    {
        _passDesc->depthAttachment()->setTexture(depthImg->texture());
        _passDesc->depthAttachment()->setLoadAction(MTL::LoadActionDontCare);
        _passDesc->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
    }
}

void CommandBuffer::clearColor(uint32_t attachmentIndex, const gpu::Color& color)
{
    (void)attachmentIndex;
    if (_passDesc)
    {
        auto* colorAtt = _passDesc->colorAttachments()->object(0);
        colorAtt->setLoadAction(MTL::LoadActionClear);
        colorAtt->setClearColor(MTL::ClearColor(color.r, color.g, color.b, color.a));
    }
}

void CommandBuffer::clearDepth(float depth)
{
    if (_passDesc)
    {
        _passDesc->depthAttachment()->setLoadAction(MTL::LoadActionClear);
        _passDesc->depthAttachment()->setClearDepth(depth);
        _passDesc->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
    }
}

void CommandBuffer::endRendering()
{
    // If the encoder was never created (clear-only frame, e.g. example 04),
    // create it now so the clear LoadAction still resolves, then end it.
    if (!_encoder && _passDesc && _cmd)
    {
        _encoder = _cmd->renderCommandEncoder(_passDesc);
    }
    if (_encoder)
    {
        _encoder->endEncoding();
        _encoder->release();
        _encoder = nullptr;
    }
    if (_passDesc)
    {
        _passDesc->release();
        _passDesc = nullptr;
    }
    _boundPipeline = nullptr;
    _renderFrame = nullptr;
}

void CommandBuffer::ensureRenderEncoder()
{
    if (_encoder)
    {
        return;
    }
    if (!_passDesc || !_cmd)
    {
        throw std::runtime_error("metal::CommandBuffer::ensureRenderEncoder: no active render pass");
    }
    _encoder = _cmd->renderCommandEncoder(_passDesc);
    if (!_encoder)
    {
        throw std::runtime_error("metal::CommandBuffer::ensureRenderEncoder: failed to create render encoder");
    }
}

void CommandBuffer::bindPipeline(gpu::GraphicsPipeline* pipeline)
{
    auto* metalPipeline = dynamic_cast<metal::GraphicsPipeline*>(pipeline);
    if (!metalPipeline)
    {
        throw std::runtime_error("metal::CommandBuffer::bindPipeline: not a metal::GraphicsPipeline");
    }

    ensureRenderEncoder();
    _encoder->setRenderPipelineState(metalPipeline->renderPipelineState());
    _boundPipeline = metalPipeline;
}

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    ensureRenderEncoder();

    MTL::PrimitiveType primType = MTL::PrimitiveTypeTriangle;
    if (_boundPipeline)
    {
        switch (_boundPipeline->topology())
        {
            case gpu::PrimitiveTopology::TriangleList:  primType = MTL::PrimitiveTypeTriangle; break;
            case gpu::PrimitiveTopology::TriangleStrip: primType = MTL::PrimitiveTypeTriangleStrip; break;
            case gpu::PrimitiveTopology::LineList:      primType = MTL::PrimitiveTypeLine; break;
            case gpu::PrimitiveTopology::PointList:     primType = MTL::PrimitiveTypePoint; break;
        }
    }

    _encoder->drawPrimitives(primType, firstVertex, vertexCount, instanceCount, firstInstance);
}

bool CommandBuffer::isValid() const
{
    return _cmd != nullptr;
}

#else

CommandBuffer::~CommandBuffer() {}
void CommandBuffer::begin() {}
void CommandBuffer::end() {}
void CommandBuffer::transition(gpu::Image*, ImageLayout) {}
void CommandBuffer::beginRendering(gpu::SurfaceFrame*) {}
void CommandBuffer::endRendering() {}
void CommandBuffer::clearColor(uint32_t, const gpu::Color&) {}
void CommandBuffer::clearDepth(float) {}
void CommandBuffer::bindPipeline(gpu::GraphicsPipeline*) {}
void CommandBuffer::draw(uint32_t, uint32_t, uint32_t, uint32_t) {}
bool CommandBuffer::isValid() const { return false; }

#endif

}
}
}
