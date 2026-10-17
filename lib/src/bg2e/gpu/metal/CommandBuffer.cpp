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
#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/gpu/metal/GraphicsPipeline.hpp>
#include <bg2e/gpu/metal/ComputePipeline.hpp>
#include <bg2e/gpu/metal/PipelineLayout.hpp>
#include <bg2e/gpu/metal/RayTracingPipeline.hpp>
#include <bg2e/gpu/metal/ResourceSet.hpp>
#include <bg2e/gpu/metal/RayTracingMesh.hpp>
#include <bg2e/gpu/metal/RayTracingScene.hpp>
#include <bg2e/gpu/metal/Image.hpp>
#include <bg2e/gpu/metal/SurfaceFrame.hpp>
#include <bg2e/gpu/CubeMap.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

MTL::CullMode metalCullMode(gpu::CullMode cullMode)
{
    switch (cullMode)
    {
    case gpu::CullMode::None:
        return MTL::CullModeNone;
    case gpu::CullMode::Front:
        return MTL::CullModeFront;
    case gpu::CullMode::Back:
        return MTL::CullModeBack;
    }
    return MTL::CullModeNone;
}

MTL::Winding metalFrontFaceWinding(gpu::FrontFace frontFace)
{
    switch (frontFace)
    {
    case gpu::FrontFace::Clockwise:
        return MTL::WindingClockwise;
    case gpu::FrontFace::CounterClockwise:
        return MTL::WindingCounterClockwise;
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
    if (_recording || _submitted)
    {
        throw std::logic_error("Metal command buffer cannot begin again");
    }
    _executable = false;
    _recording = true;
}

void CommandBuffer::end()
{
    if (!_recording || _passDesc)
    {
        throw std::logic_error("Metal command buffer must close its render scope before end");
    }
    if (_computeEncoder)
    {
        _computeEncoder->endEncoding();
        _computeEncoder->release();
        _computeEncoder = nullptr;
    }
    _boundComputePipeline = nullptr;
    _recording = false;
    _executable = true;
}

bool CommandBuffer::hasActiveScope() const
{
    return _passDesc || _encoder || _computeEncoder;
}

void CommandBuffer::transition(gpu::Image* image, ImageLayout newLayout)
{
    if (!image)
    {
        throw std::invalid_argument("Image transition requires an image");
    }
    transition(image, newLayout, image->fullRange());
}

void CommandBuffer::transition(gpu::Image* image, ImageLayout newLayout, const ImageSubresourceRange& range)
{
    if (!_recording || hasActiveScope() || !dynamic_cast<metal::Image*>(image) || !image->isValid() || image->device() != _device)
    {
        throw std::logic_error("Image transition requires a valid recording device outside active scopes");
    }
    image->validateRange(range);
    image->validateLayout(newLayout);
    // Private textures are hazard-tracked; native encoder boundaries order
    // writes on this queue. This is per-subresource API state bookkeeping.
    image->setLayout(range, newLayout);
}

void CommandBuffer::beginRendering(const RenderingDescription& description)
{
    if (!_recording || hasActiveScope() || !_device)
    {
        throw std::logic_error("Attachment rendering requires recording outside another scope");
    }
    const auto extent = validateRendering(description, *_device);
    for (const auto& attachment : description.colors)
    {
        if (!dynamic_cast<metal::Image*>(attachment.image))
        {
            throw std::invalid_argument("Color attachment is not a Metal image");
        }
    }
    if (description.depth && !dynamic_cast<metal::Image*>(description.depth->image))
    {
        throw std::invalid_argument("Depth attachment is not a Metal image");
    }
    const auto loadOperation = [](AttachmentLoadOperation operation)
    {
        switch (operation)
        {
        case AttachmentLoadOperation::Load:
            return MTL::LoadActionLoad;
        case AttachmentLoadOperation::Clear:
            return MTL::LoadActionClear;
        case AttachmentLoadOperation::DontCare:
            return MTL::LoadActionDontCare;
        }
        return MTL::LoadActionDontCare;
    };
    const auto storeOperation = [](AttachmentStoreOperation operation)
    {
        return operation == AttachmentStoreOperation::Store ? MTL::StoreActionStore : MTL::StoreActionDontCare;
    };
    _passDesc = MTL::RenderPassDescriptor::alloc()->init();
    for (size_t i = 0; i < description.colors.size(); ++i)
    {
        const auto& attachment = description.colors[i];
        auto* image = static_cast<metal::Image*>(attachment.image);
        auto* native = _passDesc->colorAttachments()->object(i);
        native->setTexture(image->texture());
        native->setLevel(attachment.subresource.baseMipLevel);
        native->setSlice(attachment.subresource.baseArrayLayer);
        native->setLoadAction(loadOperation(attachment.load));
        native->setStoreAction(storeOperation(attachment.store));
        MTL::ClearColor clear;
        if (isUnsignedIntegerFormat(image->pixelFormat()))
        {
            const auto& value = attachment.clear.unsignedInteger;
            clear = MTL::ClearColor(value[0], value[1], value[2], value[3]);
        }
        else if (isSignedIntegerFormat(image->pixelFormat()))
        {
            const auto& value = attachment.clear.signedInteger;
            clear = MTL::ClearColor(value[0], value[1], value[2], value[3]);
        }
        else
        {
            const auto& value = attachment.clear.floatingPoint;
            clear = MTL::ClearColor(value.r, value.g, value.b, value.a);
        }
        native->setClearColor(clear);
    }
    if (description.depth)
    {
        const auto& attachment = *description.depth;
        auto* image = static_cast<metal::Image*>(attachment.image);
        auto* native = _passDesc->depthAttachment();
        native->setTexture(image->texture());
        native->setLevel(attachment.subresource.baseMipLevel);
        native->setSlice(attachment.subresource.baseArrayLayer);
        native->setLoadAction(loadOperation(attachment.load));
        native->setStoreAction(storeOperation(attachment.store));
        native->setClearDepth(attachment.clearDepth);
        if (hasStencil(image->pixelFormat()))
        {
            auto* stencil = _passDesc->stencilAttachment();
            stencil->setTexture(image->texture());
            stencil->setLevel(attachment.subresource.baseMipLevel);
            stencil->setSlice(attachment.subresource.baseArrayLayer);
            stencil->setLoadAction(loadOperation(attachment.load));
            stencil->setStoreAction(storeOperation(attachment.store));
            stencil->setClearStencil(attachment.clearStencil);
        }
    }
    _attachmentRendering = description;
    _attachmentExtent = extent;
    _renderFrame = nullptr;
    _renderColorImage = nullptr;
    _renderDepthImage = nullptr;
    _renderingCubeMap = false;
    _renderCubeMap = nullptr;
}

void CommandBuffer::beginRendering(gpu::SurfaceFrame* frame)
{
    if (!_recording || hasActiveScope())
    {
        throw std::logic_error("Rendering requires recording outside another active scope");
    }
    _attachmentRendering.reset();
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
        _passDesc->depthAttachment()->setStoreAction(MTL::StoreActionStore);
    }
}

void CommandBuffer::beginRendering(gpu::Image* colorImage, uint32_t mipLevel)
{
    RenderingDescription description;
    description.colors.push_back({colorImage});
    description.colors.back().subresource = {mipLevel, 1, 0, 1};
    beginRendering(description);
}

void CommandBuffer::beginRendering(gpu::Image* colorImage, gpu::Image* depthImage, uint32_t mipLevel)
{
    RenderingDescription description;
    description.colors.push_back({colorImage});
    description.colors.back().subresource = {mipLevel, 1, 0, 1};
    if (depthImage)
    {
        description.depth = DepthAttachmentDescription{depthImage, AttachmentLoadOperation::Clear};
        description.depth->subresource = {mipLevel, 1, 0, 1};
    }
    beginRendering(description);
}

void CommandBuffer::beginRendering(gpu::CubeMap* cubemap, CubemapFace face, uint32_t mipLevel)
{
    if (!cubemap || !cubemap->isValid() || static_cast<uint32_t>(face) >= 6)
    {
        throw std::invalid_argument("Cubemap rendering requires a valid cube and face");
    }
    RenderingDescription description;
    description.colors.push_back({cubemap->image()});
    description.colors.back().subresource = {mipLevel, 1, static_cast<uint32_t>(face), 1};
    beginRendering(description);
}

void CommandBuffer::clearColor(uint32_t attachmentIndex, const gpu::Color& color)
{
    if (!_passDesc || _encoder)
    {
        throw std::logic_error("Clear color must precede render encoder materialization");
    }
    const size_t count = _attachmentRendering ? _attachmentRendering->colors.size() : 1;
    if (attachmentIndex >= count)
    {
        throw std::out_of_range("Color attachment index is out of range");
    }
    if (_attachmentRendering)
    {
        auto& attachment = _attachmentRendering->colors[attachmentIndex];
        if (isUnsignedIntegerFormat(attachment.image->pixelFormat()) || isSignedIntegerFormat(attachment.image->pixelFormat()))
        {
            throw std::invalid_argument("Use typed integer clear values in RenderingDescription");
        }
        attachment.load = AttachmentLoadOperation::Clear;
        attachment.clear.floatingPoint = color;
    }
    if (_passDesc)
    {
        auto* colorAtt = _passDesc->colorAttachments()->object(attachmentIndex);
        colorAtt->setLoadAction(MTL::LoadActionClear);
        colorAtt->setClearColor(MTL::ClearColor(color.r, color.g, color.b, color.a));
    }
}

void CommandBuffer::clearDepth(float depth)
{
    if (!_passDesc || _encoder || !std::isfinite(depth) || depth < 0.0f || depth > 1.0f)
    {
        throw std::logic_error("Clear depth must precede render encoder materialization with a valid value");
    }
    if (_attachmentRendering)
    {
        if (!_attachmentRendering->depth)
        {
            throw std::invalid_argument("No depth attachment");
        }
        _attachmentRendering->depth->load = AttachmentLoadOperation::Clear;
        _attachmentRendering->depth->clearDepth = depth;
    }
    if (_passDesc)
    {
        _passDesc->depthAttachment()->setLoadAction(MTL::LoadActionClear);
        _passDesc->depthAttachment()->setClearDepth(depth);
    }
}

void CommandBuffer::endRendering()
{
    // If the encoder was never created (clear-only frame, e.g. example 04),
    // create it now so the clear LoadAction still resolves, then end it.
    if (!_encoder && _passDesc && _cmd)
    {
        _encoder = _cmd->renderCommandEncoder(_passDesc);
        if (_encoder)
        {
            _encoder->retain(); // Balance the scope's owned release.
        }
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
    _boundLayout = nullptr;
    _attachmentRendering.reset();
    _renderFrame = nullptr;
    _renderColorImage = nullptr;
    _renderDepthImage = nullptr;
    _renderingCubeMap = false;
    _renderCubeMap = nullptr;
    _boundIndexBuffer = nullptr;
    _boundIndexBufferOffset = 0;
}

MTL::RenderPassDescriptor* CommandBuffer::renderPassDescriptor() const
{
    if (!_recording || !_passDesc || _computeEncoder)
    {
        throw std::logic_error("Metal renderPassDescriptor requires an active render scope.");
    }
    return _passDesc;
}

MTL::RenderCommandEncoder* CommandBuffer::materializeRenderEncoder()
{
    renderPassDescriptor(); // Validate the scope before emitting native commands.
    ensureRenderEncoder();
    return _encoder;
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
    if (_encoder)
    {
        _encoder->retain(); // Native result is autoreleased.
    }
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

    if (_attachmentRendering)
    {
        if (!metalPipeline->isValid() || metalPipeline->device() != _device)
        {
            throw std::invalid_argument("Pipeline must belong to the rendering device");
        }
        validatePipelineAttachments(*_attachmentRendering, *metalPipeline);
    }
    ensureRenderEncoder();
    _encoder->setRenderPipelineState(metalPipeline->renderPipelineState());
    _encoder->setCullMode(metalCullMode(metalPipeline->cullMode()));
    _encoder->setFrontFacingWinding(metalFrontFaceWinding(metalPipeline->frontFace()));
    _encoder->setDepthStencilState(metalPipeline->depthStencilState());

    if (_attachmentRendering)
    {
        MTL::Viewport viewport{0.0, 0.0, static_cast<double>(_attachmentExtent.width),
            static_cast<double>(_attachmentExtent.height), 0.0, 1.0};
        _encoder->setViewport(viewport);
        _encoder->setScissorRect(MTL::ScissorRect{0, 0, _attachmentExtent.width, _attachmentExtent.height});
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
        case gpu::PrimitiveTopology::TriangleList:
            primType = MTL::PrimitiveTypeTriangle;
            break;
        case gpu::PrimitiveTopology::TriangleStrip:
            primType = MTL::PrimitiveTypeTriangleStrip;
            break;
        case gpu::PrimitiveTopology::LineList:
            primType = MTL::PrimitiveTypeLine;
            break;
        case gpu::PrimitiveTopology::PointList:
            primType = MTL::PrimitiveTypePoint;
            break;
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
    constexpr NS::UInteger kThreadGroupWidth = 16;
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
    case ShaderStage::RayGeneration:
    case ShaderStage::Miss:
    case ShaderStage::ClosestHit:
    case ShaderStage::AnyHit:
        if (!_computeEncoder)
        {
            throw std::runtime_error("metal::CommandBuffer::pushConstants: no active compute scope; call beginCompute() first");
        }
        _computeEncoder->setBytes(data, size, binding);
        break;
    default:
        break;
    }
}

void CommandBuffer::bindResourceSet(gpu::GraphicsPipeline* /*pipeline*/, uint32_t /*setIndex*/, gpu::ResourceSet* set)
{
    auto* metalSet = dynamic_cast<metal::ResourceSet*>(set);
    if (!metalSet || metalSet->device() != _device || !metalSet->isValid())
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
        if (entry.rtScene)
        {
            // The primitive acceleration structures referenced by the instance
            // acceleration structure must be made resident before use.
            for (auto* prim : entry.rtScene->referencedPrimitives())
            {
                _encoder->useResource(prim, MTL::ResourceUsageRead);
            }
            switch (entry.stage)
            {
            case ShaderStage::Vertex:
                _encoder->setVertexAccelerationStructure(entry.rtScene->handle(), entry.index);
                break;
            case ShaderStage::Fragment:
                _encoder->setFragmentAccelerationStructure(entry.rtScene->handle(), entry.index);
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
    if (!metalSet || metalSet->device() != _device || !metalSet->isValid())
    {
        throw std::runtime_error("metal::CommandBuffer::bindResourceSet(ComputePipeline): not a metal::ResourceSet");
    }

    for (const auto& entry : metalSet->entries())
    {
        for (auto* texture : entry.indirectTextures)
        {
            _computeEncoder->useResource(texture, MTL::ResourceUsageRead);
        }
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
        if (entry.rtScene)
        {
            for (auto* prim : entry.rtScene->referencedPrimitives())
            {
                _computeEncoder->useResource(prim, MTL::ResourceUsageRead);
            }
            _computeEncoder->setAccelerationStructure(entry.rtScene->handle(), entry.index);
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

    _boundIndexBuffer = metalBuf->handle();
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
        case gpu::PrimitiveTopology::TriangleList:
            primType = MTL::PrimitiveTypeTriangle;
            break;
        case gpu::PrimitiveTopology::TriangleStrip:
            primType = MTL::PrimitiveTypeTriangleStrip;
            break;
        case gpu::PrimitiveTopology::LineList:
            primType = MTL::PrimitiveTypeLine;
            break;
        case gpu::PrimitiveTopology::PointList:
            primType = MTL::PrimitiveTypePoint;
            break;
        }
    }

    NS::UInteger byteOffset = _boundIndexBufferOffset + static_cast<NS::UInteger>(firstIndex) * sizeof(uint32_t);

    _encoder->drawIndexedPrimitives(
        primType,
        static_cast<NS::UInteger>(indexCount),
        MTL::IndexTypeUInt32,
        _boundIndexBuffer,
        byteOffset,
        static_cast<NS::UInteger>(instanceCount),
        static_cast<NS::Integer>(vertexOffset),
        static_cast<NS::UInteger>(firstInstance));
}

void CommandBuffer::copyImage(gpu::Image* source, gpu::Image* destination)
{
    copyImage(ImageView{source, {}}, ImageView{destination, {}});
}

void CommandBuffer::copyImage(const ImageView& source, const ImageView& destination)
{
    if (!_recording || hasActiveScope() || !_device || !_cmd)
    {
        throw std::logic_error("Image copies require recording outside active scopes");
    }
    validateCopy(source, destination, *_device);
    auto* nativeSource = dynamic_cast<metal::Image*>(source.image);
    auto* nativeDestination = dynamic_cast<metal::Image*>(destination.image);
    if (!nativeSource || !nativeDestination)
    {
        throw std::invalid_argument("Image copy requires Metal images");
    }
    const auto extent = source.image->mipExtent(source.range.baseMipLevel);
    auto* encoder = _cmd->blitCommandEncoder();
    if (!encoder)
    {
        throw std::runtime_error("Metal copy blit encoder creation failed");
    }
    encoder->copyFromTexture(nativeSource->texture(), source.range.baseArrayLayer, source.range.baseMipLevel,
        MTL::Origin{0, 0, 0}, MTL::Size{extent.width, extent.height, 1}, nativeDestination->texture(),
        destination.range.baseArrayLayer, destination.range.baseMipLevel, MTL::Origin{0, 0, 0});
    encoder->endEncoding();
}

void CommandBuffer::generateMipmaps(gpu::Image* image, ImageLayout finalLayout)
{
    auto* native = dynamic_cast<metal::Image*>(image);
    if (!_recording || hasActiveScope() || !native || !_device || !_cmd)
    {
        throw std::logic_error("Mipmap generation requires recording outside active scopes");
    }
    validateMipmapGeneration(*image, *_device, finalLayout);
    if (image->mipLevels() > 1)
    {
        transition(image, ImageLayout::TransferSrc, {0, 1, 0, image->arrayLayers()});
        transition(image, ImageLayout::TransferDst, {1, image->mipLevels() - 1, 0, image->arrayLayers()});
        auto* encoder = _cmd->blitCommandEncoder();
        if (!encoder)
        {
            throw std::runtime_error("Metal mipmap blit encoder creation failed");
        }
        encoder->generateMipmaps(native->texture());
        encoder->endEncoding();
    }
    transition(image, finalLayout);
}

void CommandBuffer::blitImage(gpu::Image* source, gpu::Image* destination)
{
    if (!source || !destination || source->size() != destination->size())
    {
        throw std::invalid_argument("Metal blitImage does not rescale textures; use a rendering/compute resampler");
    }
    copyImage(source, destination);
}

void CommandBuffer::buildRayTracingMesh(gpu::RayTracingMesh* mesh)
{
    auto* metalMesh = dynamic_cast<metal::RayTracingMesh*>(mesh);
    if (!metalMesh)
    {
        throw std::runtime_error("metal::CommandBuffer::buildRayTracingMesh: not a metal::RayTracingMesh");
    }
    if (_encoder || _computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::buildRayTracingMesh: an encoding scope is still active");
    }
    metalMesh->build(_cmd);
}

void CommandBuffer::buildRayTracingScene(gpu::RayTracingScene* scene)
{
    auto* metalScene = dynamic_cast<metal::RayTracingScene*>(scene);
    if (!metalScene)
    {
        throw std::runtime_error("metal::CommandBuffer::buildRayTracingScene: not a metal::RayTracingScene");
    }
    if (_encoder || _computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::buildRayTracingScene: an encoding scope is still active");
    }
    metalScene->build(_cmd);
}

void CommandBuffer::bindPipeline(gpu::RayTracingPipeline* pipeline)
{
    auto* metalPipeline = dynamic_cast<metal::RayTracingPipeline*>(pipeline);
    if (!metalPipeline)
    {
        throw std::runtime_error("metal::CommandBuffer::bindPipeline(RayTracingPipeline): not a metal::RayTracingPipeline");
    }

    // Create compute encoder if not already active
    if (!_computeEncoder)
    {
        if (!_cmd)
        {
            throw std::runtime_error("metal::CommandBuffer::bindPipeline(RayTracingPipeline): no command buffer");
        }
        _computeEncoder = _cmd->computeCommandEncoder();
        if (!_computeEncoder)
        {
            throw std::runtime_error("metal::CommandBuffer::bindPipeline(RayTracingPipeline): failed to create compute encoder");
        }
    }

    _computeEncoder->setComputePipelineState(metalPipeline->computePipelineState());
    _boundComputePipeline = nullptr;
    _boundRayTracingPipeline = metalPipeline;
    _boundLayout = metalPipeline->layout();
}

void CommandBuffer::bindResourceSet(gpu::RayTracingPipeline* /*pipeline*/, uint32_t /*setIndex*/, gpu::ResourceSet* set)
{
    if (!_computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::bindResourceSet(RayTracingPipeline): no active compute scope");
    }

    auto* metalSet = dynamic_cast<metal::ResourceSet*>(set);
    if (!metalSet || metalSet->device() != _device || !metalSet->isValid())
    {
        throw std::runtime_error("metal::CommandBuffer::bindResourceSet(RayTracingPipeline): not a metal::ResourceSet");
    }

    // Reuse the same resource binding logic as compute pipelines
    for (const auto& entry : metalSet->entries())
    {
        for (auto* texture : entry.indirectTextures)
        {
            _computeEncoder->useResource(texture, MTL::ResourceUsageRead);
        }
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
        if (entry.rtScene)
        {
            for (auto* prim : entry.rtScene->referencedPrimitives())
            {
                _computeEncoder->useResource(prim, MTL::ResourceUsageRead);
            }
            _computeEncoder->setAccelerationStructure(entry.rtScene->handle(), entry.index);
        }
    }
}

void CommandBuffer::traceRays(uint32_t width, uint32_t height, uint32_t depth)
{
    if (!_computeEncoder)
    {
        throw std::runtime_error("metal::CommandBuffer::traceRays: no active compute scope; call bindPipeline(RayTracingPipeline) first");
    }

    // Use 8x8 threadgroups to match the Metal shader threadgroup_size
    constexpr NS::UInteger kThreadGroupWidth = 8;
    constexpr NS::UInteger kThreadGroupHeight = 8;

    MTL::Size threadsPerGroup = MTL::Size::Make(kThreadGroupWidth, kThreadGroupHeight, 1);
    MTL::Size threadGroups = MTL::Size::Make(
        (width + kThreadGroupWidth - 1) / kThreadGroupWidth,
        (height + kThreadGroupHeight - 1) / kThreadGroupHeight,
        depth);
    _computeEncoder->dispatchThreadgroups(threadGroups, threadsPerGroup);
}

bool CommandBuffer::isValid() const
{
    return _cmd != nullptr;
}

#else

CommandBuffer::~CommandBuffer()
{
}

void CommandBuffer::begin()
{
}

void CommandBuffer::end()
{
}

bool CommandBuffer::hasActiveScope() const
{
    return false;
}

void CommandBuffer::transition(gpu::Image*, ImageLayout, const ImageSubresourceRange&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void CommandBuffer::copyImage(const ImageView&, const ImageView&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void CommandBuffer::generateMipmaps(gpu::Image*, ImageLayout)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void CommandBuffer::transition(gpu::Image*, ImageLayout)
{
}

void CommandBuffer::beginRendering(const RenderingDescription&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}

void CommandBuffer::beginRendering(gpu::SurfaceFrame*)
{
    if (_attachmentRendering)
    {
        throw std::logic_error("Close attachment rendering before opening another scope");
    }
    _attachmentRendering.reset();
}

void CommandBuffer::beginRendering(gpu::Image*, uint32_t)
{
    if (_attachmentRendering)
    {
        throw std::logic_error("Close attachment rendering before opening another scope");
    }
    _attachmentRendering.reset();
}

void CommandBuffer::beginRendering(gpu::Image*, gpu::Image*, uint32_t)
{
    if (_attachmentRendering)
    {
        throw std::logic_error("Close attachment rendering before opening another scope");
    }
    _attachmentRendering.reset();
}

void CommandBuffer::beginRendering(gpu::CubeMap*, CubemapFace, uint32_t)
{
    if (_attachmentRendering)
    {
        throw std::logic_error("Close attachment rendering before opening another scope");
    }
    _attachmentRendering.reset();
}

void CommandBuffer::endRendering()
{
}

void CommandBuffer::beginCompute()
{
}

void CommandBuffer::endCompute()
{
}

void CommandBuffer::clearColor(uint32_t, const gpu::Color&)
{
}

void CommandBuffer::clearDepth(float)
{
}

void CommandBuffer::bindPipeline(gpu::GraphicsPipeline*)
{
}

void CommandBuffer::draw(uint32_t, uint32_t, uint32_t, uint32_t)
{
}

void CommandBuffer::bindPipeline(gpu::ComputePipeline*)
{
}

void CommandBuffer::dispatch(uint32_t, uint32_t, uint32_t)
{
}

void CommandBuffer::pushConstants(ShaderStage, uint32_t, uint32_t, const void*)
{
}

void CommandBuffer::bindResourceSet(gpu::GraphicsPipeline*, uint32_t, gpu::ResourceSet*)
{
}

void CommandBuffer::bindResourceSet(gpu::ComputePipeline*, uint32_t, gpu::ResourceSet*)
{
}

void CommandBuffer::bindVertexBuffer(uint32_t, gpu::Buffer*, uint64_t)
{
}

void CommandBuffer::bindIndexBuffer(gpu::Buffer*, uint64_t)
{
}

void CommandBuffer::drawIndexed(uint32_t, uint32_t, uint32_t, int32_t, uint32_t)
{
}

void CommandBuffer::copyImage(gpu::Image*, gpu::Image*)
{
}

void CommandBuffer::blitImage(gpu::Image*, gpu::Image*)
{
}

void CommandBuffer::buildRayTracingMesh(gpu::RayTracingMesh*)
{
}

void CommandBuffer::buildRayTracingScene(gpu::RayTracingScene*)
{
}

void CommandBuffer::bindPipeline(gpu::RayTracingPipeline*)
{
}

void CommandBuffer::bindResourceSet(gpu::RayTracingPipeline*, uint32_t, gpu::ResourceSet*)
{
}

void CommandBuffer::traceRays(uint32_t, uint32_t, uint32_t)
{
}

bool CommandBuffer::isValid() const
{
    return false;
}

#endif

}
}
}
