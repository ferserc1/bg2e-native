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
#include <bg2e/gpu/metal/Buffer.hpp>
#include <bg2e/gpu/metal/GraphicsPipeline.hpp>
#include <bg2e/gpu/metal/ComputePipeline.hpp>
#include <bg2e/gpu/metal/PipelineLayout.hpp>
#include <bg2e/gpu/metal/ResourceSet.hpp>
#include <bg2e/gpu/metal/Image.hpp>
#include <bg2e/gpu/metal/SurfaceFrame.hpp>
#include <bg2e/gpu/CubeMap.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>

#include <algorithm>
#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

MTL::CullMode metalCullMode(gpu::CullMode cullMode)
{
    switch (cullMode)
    {
        case gpu::CullMode::None:   return MTL::CullModeNone;
        case gpu::CullMode::Front:  return MTL::CullModeFront;
        case gpu::CullMode::Back:   return MTL::CullModeBack;
    }
    return MTL::CullModeNone;
}

MTL::Winding metalFrontFaceWinding(gpu::FrontFace frontFace)
{
    switch (frontFace)
    {
        case gpu::FrontFace::Clockwise:            return MTL::WindingClockwise;
        case gpu::FrontFace::CounterClockwise:     return MTL::WindingCounterClockwise;
    }
    return MTL::WindingCounterClockwise;
}

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
    if (_computeEncoder)
    {
        _computeEncoder->release();
        _computeEncoder = nullptr;
    }
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
    if (_computeEncoder)
    {
        _computeEncoder->endEncoding();
        _computeEncoder->release();
        _computeEncoder = nullptr;
    }
    _boundComputePipeline = nullptr;
    _recording = false;
}

void CommandBuffer::transition(gpu::Image* image, ImageLayout newLayout)
{
    // Metal manages hazards automatically for render targets.
    // This is bookkeeping only for validation/debug.
    image->_currentLayout = newLayout;
}

void CommandBuffer::beginRendering(gpu::SurfaceFrame* frame)
{
    if (_computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::beginRendering: a compute scope is still active; call endCompute() before beginRendering()");
    }

    _renderFrame = dynamic_cast<metal::SurfaceFrame*>(frame);
    if (!_renderFrame)
    {
        throw std::runtime_error("metal::CommandBuffer::beginRendering: not a metal::SurfaceFrame");
    }

    _renderColorImage = nullptr;
    _renderDepthImage = nullptr;
    _renderingCubeMap = false;
    _renderCubeMap = nullptr;

    _passDesc = MTL::RenderPassDescriptor::alloc()->init();

    auto* colorImg = dynamic_cast<metal::Image*>(_renderFrame->colorImage());
    if (colorImg && colorImg->texture())
    {
        _passDesc->colorAttachments()->object(0)->setTexture(colorImg->texture());
        _passDesc->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
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

void CommandBuffer::beginRendering(gpu::Image* colorImage, uint32_t /*mipLevel*/)
{
    if (_computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::beginRendering: a compute scope is still active; call endCompute() before beginRendering()");
    }

    _renderFrame = nullptr;
    _renderColorImage = colorImage;
    _renderDepthImage = nullptr;
    _renderingCubeMap = false;
    _renderCubeMap = nullptr;

    _passDesc = MTL::RenderPassDescriptor::alloc()->init();

    auto* colorImg = dynamic_cast<metal::Image*>(colorImage);
    if (colorImg && colorImg->texture())
    {
        _passDesc->colorAttachments()->object(0)->setTexture(colorImg->texture());
        _passDesc->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
        _passDesc->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
    }
}

void CommandBuffer::beginRendering(gpu::Image* colorImage, gpu::Image* depthImage, uint32_t /*mipLevel*/)
{
    if (_computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::beginRendering: a compute scope is still active; call endCompute() before beginRendering()");
    }

    _renderFrame = nullptr;
    _renderColorImage = colorImage;
    _renderDepthImage = depthImage;
    _renderingCubeMap = false;
    _renderCubeMap = nullptr;

    _passDesc = MTL::RenderPassDescriptor::alloc()->init();

    auto* colorImg = dynamic_cast<metal::Image*>(colorImage);
    if (colorImg && colorImg->texture())
    {
        _passDesc->colorAttachments()->object(0)->setTexture(colorImg->texture());
        _passDesc->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
        _passDesc->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
    }

    auto* depthImg = dynamic_cast<metal::Image*>(depthImage);
    if (depthImg && depthImg->isValid() && depthImg->texture())
    {
        _passDesc->depthAttachment()->setTexture(depthImg->texture());
        _passDesc->depthAttachment()->setLoadAction(MTL::LoadActionDontCare);
        _passDesc->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
    }
}

void CommandBuffer::beginRendering(gpu::CubeMap* cubemap, CubemapFace face, uint32_t mipLevel)
{
    if (_computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::beginRendering(CubeMap): a compute scope is still active; call endCompute() before beginRendering()");
    }
    if (!cubemap)
    {
        throw std::runtime_error("metal::CommandBuffer::beginRendering(CubeMap): cubemap is null");
    }
    if (!cubemap->isValid())
    {
        throw std::runtime_error("metal::CommandBuffer::beginRendering(CubeMap): cubemap is not valid");
    }
    if (!cubemap->image())
    {
        throw std::runtime_error("metal::CommandBuffer::beginRendering(CubeMap): cubemap image is null");
    }
    if (!cubemap->image()->isCubemap())
    {
        throw std::runtime_error("metal::CommandBuffer::beginRendering(CubeMap): image is not a cubemap");
    }
    if (static_cast<uint32_t>(face) > 5)
    {
        throw std::runtime_error("metal::CommandBuffer::beginRendering(CubeMap): face must be in [0, 5]");
    }
    if (mipLevel >= cubemap->mipLevels())
    {
        throw std::runtime_error("metal::CommandBuffer::beginRendering(CubeMap): mipLevel out of range");
    }

    auto* metalImage = dynamic_cast<metal::Image*>(cubemap->image());
    if (!metalImage || !metalImage->isValidCubemapFaceMip(static_cast<uint32_t>(face), mipLevel))
    {
        throw std::runtime_error("metal::CommandBuffer::beginRendering(CubeMap): invalid cubemap face/mip");
    }

    _renderFrame = nullptr;
    _renderColorImage = nullptr;
    _renderDepthImage = nullptr;
    _renderCubeMap = cubemap;
    _renderCubeFace = static_cast<uint32_t>(face);
    _renderCubeMipLevel = mipLevel;
    _renderingCubeMap = true;

    _passDesc = MTL::RenderPassDescriptor::alloc()->init();

    auto target = metalImage->cubemapFaceMipTarget(static_cast<uint32_t>(face), mipLevel);
    _passDesc->colorAttachments()->object(0)->setTexture(target.texture);
    _passDesc->colorAttachments()->object(0)->setSlice(target.face);
    _passDesc->colorAttachments()->object(0)->setLevel(target.mipLevel);
    _passDesc->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
    _passDesc->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
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
    _boundPipeline        = nullptr;
    _boundLayout          = nullptr;
    _renderFrame          = nullptr;
    _renderColorImage     = nullptr;
    _renderDepthImage     = nullptr;
    _renderingCubeMap     = false;
    _renderCubeMap        = nullptr;
    _boundIndexBuffer     = nullptr;
    _boundIndexBufferOffset = 0;
}

void CommandBuffer::ensureRenderEncoder()
{
    if (_encoder)
    {
        return;
    }
    if (_computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::ensureRenderEncoder: a compute scope is still active; call endCompute() before rendering");
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
    _encoder->setCullMode(metalCullMode(metalPipeline->cullMode()));
    _encoder->setFrontFacingWinding(metalFrontFaceWinding(metalPipeline->frontFace()));
    if (metalPipeline->depthStencilState())
    {
        _encoder->setDepthStencilState(metalPipeline->depthStencilState());
    }

    if (_renderingCubeMap && _renderCubeMap)
    {
        uint32_t mipSize = std::max(1u, _renderCubeMap->size() >> _renderCubeMipLevel);
        MTL::Viewport viewport;
        viewport.originX = 0.0;
        viewport.originY = 0.0;
        viewport.width = static_cast<double>(mipSize);
        viewport.height = static_cast<double>(mipSize);
        viewport.znear = 0.0;
        viewport.zfar = 1.0;
        _encoder->setViewport(viewport);

        MTL::ScissorRect scissor;
        scissor.x = 0;
        scissor.y = 0;
        scissor.width = mipSize;
        scissor.height = mipSize;
        _encoder->setScissorRect(scissor);
    }

    _boundPipeline = metalPipeline;
    _boundLayout = metalPipeline->layout();
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

void CommandBuffer::beginCompute()
{
    if (_computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::beginCompute: a compute scope is already active");
    }
    if (_encoder)
    {
        throw std::runtime_error("metal::CommandBuffer::beginCompute: cannot begin a compute scope while a rendering scope is active");
    }
    if (!_cmd)
    {
        throw std::runtime_error("metal::CommandBuffer::beginCompute: no command buffer");
    }
    _computeEncoder = _cmd->computeCommandEncoder();
    if (!_computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::beginCompute: failed to create compute encoder");
    }
}

void CommandBuffer::endCompute()
{
    if (!_computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::endCompute: no active compute scope; call beginCompute() first");
    }
    _computeEncoder->endEncoding();
    _computeEncoder->release();
    _computeEncoder = nullptr;
    _boundComputePipeline = nullptr;
}

void CommandBuffer::bindPipeline(gpu::ComputePipeline* pipeline)
{
    if (!_computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::bindPipeline(ComputePipeline): no active compute scope; call beginCompute() first");
    }

    auto* metalPipeline = dynamic_cast<metal::ComputePipeline*>(pipeline);
    if (!metalPipeline)
    {
        throw std::runtime_error("metal::CommandBuffer::bindPipeline(ComputePipeline): not a metal::ComputePipeline");
    }

    _computeEncoder->setComputePipelineState(metalPipeline->computePipelineState());
    _boundComputePipeline = metalPipeline;
    _boundLayout = metalPipeline->layout();
}

void CommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    if (!_computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::dispatch: no active compute scope; call beginCompute() first");
    }

    // Use 16x16 threads-per-group to match the GLSL local_size_x/y convention.
    // The groupCountX/Y/Z parameters are treated as threadgroup counts (matching
    // Vulkan's dispatch model). Compute kernels that need a different threadgroup
    // size can be added later; for now 16x16 covers the gradient and typical
    // image-processing dispatches.
    constexpr NS::UInteger kThreadGroupWidth  = 16;
    constexpr NS::UInteger kThreadGroupHeight = 16;

    MTL::Size threadsPerGroup = MTL::Size::Make(kThreadGroupWidth, kThreadGroupHeight, 1);
    MTL::Size threadGroups = MTL::Size::Make(groupCountX, groupCountY, groupCountZ);
    _computeEncoder->dispatchThreadgroups(threadGroups, threadsPerGroup);
}

void CommandBuffer::pushConstants(ShaderStage stage, uint32_t offset, uint32_t size, const void* data)
{
    (void)offset;

    if (!_boundLayout)
    {
        throw std::runtime_error("metal::CommandBuffer::pushConstants: no pipeline bound");
    }

    auto binding = _boundLayout->pushConstantBufferIndex(stage);
    switch (stage)
    {
        case ShaderStage::Vertex:
            ensureRenderEncoder();
            _encoder->setVertexBytes(data, size, binding);
            break;
        case ShaderStage::Fragment:
            ensureRenderEncoder();
            _encoder->setFragmentBytes(data, size, binding);
            break;
        case ShaderStage::Compute:
            if (!_computeEncoder)
            {
                throw std::runtime_error("metal::CommandBuffer::pushConstants: no active compute scope; call beginCompute() first");
            }
            _computeEncoder->setBytes(data, size, binding);
            break;
    }
}

void CommandBuffer::bindResourceSet(gpu::GraphicsPipeline* /*pipeline*/, uint32_t /*setIndex*/, gpu::ResourceSet* set)
{
    auto* metalSet = dynamic_cast<metal::ResourceSet*>(set);
    if (!metalSet)
    {
        throw std::runtime_error("metal::CommandBuffer::bindResourceSet(GraphicsPipeline): not a metal::ResourceSet");
    }

    ensureRenderEncoder();

    for (const auto& entry : metalSet->entries())
    {
        if (entry.texture)
        {
            switch (entry.stage)
            {
                case ShaderStage::Vertex:
                    _encoder->setVertexTexture(entry.texture, entry.index);
                    break;
                case ShaderStage::Fragment:
                    _encoder->setFragmentTexture(entry.texture, entry.index);
                    break;
                default:
                    break;
            }
        }
        if (entry.sampler)
        {
            switch (entry.stage)
            {
                case ShaderStage::Vertex:
                    _encoder->setVertexSamplerState(entry.sampler, entry.index);
                    break;
                case ShaderStage::Fragment:
                    _encoder->setFragmentSamplerState(entry.sampler, entry.index);
                    break;
                default:
                    break;
            }
        }
        if (entry.buffer)
        {
            switch (entry.stage)
            {
                case ShaderStage::Vertex:
                    _encoder->setVertexBuffer(entry.buffer, 0, entry.index);
                    break;
                case ShaderStage::Fragment:
                    _encoder->setFragmentBuffer(entry.buffer, 0, entry.index);
                    break;
                default:
                    break;
            }
        }
    }
}

void CommandBuffer::bindResourceSet(gpu::ComputePipeline* /*pipeline*/, uint32_t /*setIndex*/, gpu::ResourceSet* set)
{
    if (!_computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::bindResourceSet(ComputePipeline): no active compute scope; call beginCompute() first");
    }

    auto* metalSet = dynamic_cast<metal::ResourceSet*>(set);
    if (!metalSet)
    {
        throw std::runtime_error("metal::CommandBuffer::bindResourceSet(ComputePipeline): not a metal::ResourceSet");
    }

    for (const auto& entry : metalSet->entries())
    {
        if (entry.texture)
        {
            _computeEncoder->setTexture(entry.texture, entry.index);
        }
        if (entry.sampler)
        {
            _computeEncoder->setSamplerState(entry.sampler, entry.index);
        }
        if (entry.buffer)
        {
            _computeEncoder->setBuffer(entry.buffer, 0, entry.index);
        }
    }
}

void CommandBuffer::bindVertexBuffer(uint32_t binding, gpu::Buffer* buffer, uint64_t offset)
{
    auto* metalBuf = dynamic_cast<metal::Buffer*>(buffer);
    if (!metalBuf)
    {
        throw std::runtime_error("metal::CommandBuffer::bindVertexBuffer: not a metal::Buffer");
    }

    ensureRenderEncoder();
    _encoder->setVertexBuffer(metalBuf->handle(), static_cast<NS::UInteger>(offset), binding);
}

void CommandBuffer::bindIndexBuffer(gpu::Buffer* buffer, uint64_t offset)
{
    auto* metalBuf = dynamic_cast<metal::Buffer*>(buffer);
    if (!metalBuf)
    {
        throw std::runtime_error("metal::CommandBuffer::bindIndexBuffer: not a metal::Buffer");
    }

    _boundIndexBuffer       = metalBuf->handle();
    _boundIndexBufferOffset = static_cast<NS::UInteger>(offset);
}

void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                uint32_t firstIndex, int32_t vertexOffset,
                                uint32_t firstInstance)
{
    ensureRenderEncoder();

    if (!_boundIndexBuffer)
    {
        throw std::runtime_error("metal::CommandBuffer::drawIndexed: no index buffer bound");
    }

    MTL::PrimitiveType primType = MTL::PrimitiveTypeTriangle;
    if (_boundPipeline)
    {
        switch (_boundPipeline->topology())
        {
            case gpu::PrimitiveTopology::TriangleList:  primType = MTL::PrimitiveTypeTriangle;     break;
            case gpu::PrimitiveTopology::TriangleStrip: primType = MTL::PrimitiveTypeTriangleStrip; break;
            case gpu::PrimitiveTopology::LineList:      primType = MTL::PrimitiveTypeLine;          break;
            case gpu::PrimitiveTopology::PointList:     primType = MTL::PrimitiveTypePoint;         break;
        }
    }

    NS::UInteger byteOffset = _boundIndexBufferOffset
                            + static_cast<NS::UInteger>(firstIndex) * sizeof(uint32_t);

    _encoder->drawIndexedPrimitives(
        primType,
        static_cast<NS::UInteger>(indexCount),
        MTL::IndexTypeUInt32,
        _boundIndexBuffer,
        byteOffset,
        static_cast<NS::UInteger>(instanceCount),
        static_cast<NS::Integer>(vertexOffset),
        static_cast<NS::UInteger>(firstInstance)
    );
}

void CommandBuffer::copyImage(gpu::Image* src, gpu::Image* dst)
{
    auto* metalSrc = dynamic_cast<metal::Image*>(src);
    auto* metalDst = dynamic_cast<metal::Image*>(dst);
    if (!metalSrc || !metalDst)
    {
        throw std::runtime_error("metal::CommandBuffer::copyImage: not a metal::Image");
    }

    // End any active encoder before starting blit
    if (_encoder)
    {
        _encoder->endEncoding();
        _encoder->release();
        _encoder = nullptr;
    }
    if (_computeEncoder)
    {
        _computeEncoder->endEncoding();
        _computeEncoder->release();
        _computeEncoder = nullptr;
    }

    auto* blitEncoder = _cmd->blitCommandEncoder();
    blitEncoder->copyFromTexture(metalSrc->texture(), metalDst->texture());
    blitEncoder->endEncoding();
    blitEncoder->release();
}

// TODO: function not fully tested
void CommandBuffer::blitImage(gpu::Image* src, gpu::Image* dst)
{
    auto* metalSrc = dynamic_cast<metal::Image*>(src);
    auto* metalDst = dynamic_cast<metal::Image*>(dst);
    if (!metalSrc || !metalDst)
    {
        throw std::runtime_error("metal::CommandBuffer::blitImage: not a metal::Image");
    }

    // End any active encoder before starting blit
    if (_encoder)
    {
        _encoder->endEncoding();
        _encoder->release();
        _encoder = nullptr;
    }
    if (_computeEncoder)
    {
        _computeEncoder->endEncoding();
        _computeEncoder->release();
        _computeEncoder = nullptr;
    }

    auto* blitEncoder = _cmd->blitCommandEncoder();

    MTL::Origin srcOrigin(0, 0, 0);
    MTL::Size srcSize(src->width(), src->height(), 1);
    MTL::Origin dstOrigin(0, 0, 0);
    MTL::Size dstSize(dst->width(), dst->height(), 1);

    blitEncoder->copyFromTexture(
        metalSrc->texture(), 0, 0, srcOrigin, srcSize,
        metalDst->texture(), 0, 0, dstOrigin
    );
    blitEncoder->endEncoding();
    blitEncoder->release();
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
void CommandBuffer::beginRendering(gpu::Image*, uint32_t) {}
void CommandBuffer::beginRendering(gpu::Image*, gpu::Image*, uint32_t) {}
void CommandBuffer::beginRendering(gpu::CubeMap*, CubemapFace, uint32_t) {}
void CommandBuffer::endRendering() {}
void CommandBuffer::beginCompute() {}
void CommandBuffer::endCompute() {}
void CommandBuffer::clearColor(uint32_t, const gpu::Color&) {}
void CommandBuffer::clearDepth(float) {}
void CommandBuffer::bindPipeline(gpu::GraphicsPipeline*) {}
void CommandBuffer::draw(uint32_t, uint32_t, uint32_t, uint32_t) {}
void CommandBuffer::bindPipeline(gpu::ComputePipeline*) {}
void CommandBuffer::dispatch(uint32_t, uint32_t, uint32_t) {}
void CommandBuffer::pushConstants(ShaderStage, uint32_t, uint32_t, const void*) {}
void CommandBuffer::bindResourceSet(gpu::GraphicsPipeline*, uint32_t, gpu::ResourceSet*) {}
void CommandBuffer::bindResourceSet(gpu::ComputePipeline*, uint32_t, gpu::ResourceSet*) {}
void CommandBuffer::bindVertexBuffer(uint32_t, gpu::Buffer*, uint64_t) {}
void CommandBuffer::bindIndexBuffer(gpu::Buffer*, uint64_t) {}
void CommandBuffer::drawIndexed(uint32_t, uint32_t, uint32_t, int32_t, uint32_t) {}
void CommandBuffer::copyImage(gpu::Image*, gpu::Image*) {}
void CommandBuffer::blitImage(gpu::Image*, gpu::Image*) {}
bool CommandBuffer::isValid() const { return false; }

#endif

}
}
}
