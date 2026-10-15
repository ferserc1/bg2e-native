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

#include <bg2e/gpu/metal/GraphicsPipeline.hpp>
#include <bg2e/gpu/metal/ShaderModule.hpp>
#include <bg2e/gpu/metal/PipelineLayout.hpp>
#include <bg2e/gpu/metal/VertexDescriptor.hpp>
#include <bg2e/base/Log.hpp>

#include <stdexcept>
#include <bg2e/gpu/Device.hpp>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

MTL::PrimitiveType primitiveTopologyToMetal(gpu::PrimitiveTopology topology)
{
    switch (topology)
    {
    case gpu::PrimitiveTopology::TriangleList:
        return MTL::PrimitiveTypeTriangle;
    case gpu::PrimitiveTopology::TriangleStrip:
        return MTL::PrimitiveTypeTriangleStrip;
    case gpu::PrimitiveTopology::LineList:
        return MTL::PrimitiveTypeLine;
    case gpu::PrimitiveTopology::PointList:
        return MTL::PrimitiveTypePoint;
    }
    return MTL::PrimitiveTypeTriangle;
}

MTL::CompareFunction compareOperation(CompareOperation value)
{
    const MTL::CompareFunction values[] = {MTL::CompareFunctionNever, MTL::CompareFunctionLess, MTL::CompareFunctionEqual, MTL::CompareFunctionLessEqual, MTL::CompareFunctionGreater, MTL::CompareFunctionNotEqual, MTL::CompareFunctionGreaterEqual, MTL::CompareFunctionAlways};
    return values[static_cast<size_t>(value)];
}
MTL::BlendFactor blendFactor(BlendFactor value)
{
    const MTL::BlendFactor values[] = {MTL::BlendFactorZero, MTL::BlendFactorOne, MTL::BlendFactorSourceColor, MTL::BlendFactorOneMinusSourceColor, MTL::BlendFactorDestinationColor, MTL::BlendFactorOneMinusDestinationColor, MTL::BlendFactorSourceAlpha, MTL::BlendFactorOneMinusSourceAlpha, MTL::BlendFactorDestinationAlpha, MTL::BlendFactorOneMinusDestinationAlpha};
    return values[static_cast<size_t>(value)];
}
MTL::BlendOperation blendOperation(BlendOperation value)
{
    const MTL::BlendOperation values[] = {MTL::BlendOperationAdd, MTL::BlendOperationSubtract, MTL::BlendOperationReverseSubtract, MTL::BlendOperationMin, MTL::BlendOperationMax};
    return values[static_cast<size_t>(value)];
}
MTL::ColorWriteMask colorWriteMask(ColorWriteMask value)
{
    uint32_t result = 0;
    const auto mask = static_cast<uint32_t>(value);
    if (mask & 1u)
    {
        result |= MTL::ColorWriteMaskRed;
    }
    if (mask & 2u)
    {
        result |= MTL::ColorWriteMaskGreen;
    }
    if (mask & 4u)
    {
        result |= MTL::ColorWriteMaskBlue;
    }
    if (mask & 8u)
    {
        result |= MTL::ColorWriteMaskAlpha;
    }
    return static_cast<MTL::ColorWriteMask>(result);
}

GraphicsPipeline::GraphicsPipeline(gpu::Device* gpuDevice, MTL::Device* device, const gpu::GraphicsPipelineDescription& description)
    : gpu::GraphicsPipeline(gpuDevice), _topology(description.topology), _layout(dynamic_cast<metal::PipelineLayout*>(description.layout)), _cullMode(description.cullMode), _frontFace(description.frontFace)
{
    if (!gpuDevice)
    {
        throw std::invalid_argument("Graphics pipeline requires a device");
    }
    validateDescription(description, *gpuDevice);
    setAttachmentDescription(description);

    auto* metalVsModule = dynamic_cast<metal::ShaderModule*>(description.vertexShader);
    auto* metalFsModule = dynamic_cast<metal::ShaderModule*>(description.fragmentShader);

    if (!metalVsModule || !metalFsModule)
    {
        throw std::runtime_error("metal::GraphicsPipeline: description contains non-Metal objects");
    }

    auto* descriptor = MTL::RenderPipelineDescriptor::alloc();
    descriptor->init();

    descriptor->setVertexFunction(metalVsModule->function());
    descriptor->setFragmentFunction(metalFsModule->function());
    descriptor->setLabel(NS::String::string(description.debugName.c_str(), NS::UTF8StringEncoding));

    for (size_t i = 0; i < _colorAttachments.size(); ++i)
    {
        const auto& attachment = _colorAttachments[i];
        auto* target = descriptor->colorAttachments()->object(i);
        target->setPixelFormat(toMetalPixelFormat(attachment.format));
        target->setBlendingEnabled(attachment.blend.enabled);
        target->setSourceRGBBlendFactor(blendFactor(attachment.blend.sourceColor));
        target->setDestinationRGBBlendFactor(blendFactor(attachment.blend.destinationColor));
        target->setRgbBlendOperation(blendOperation(attachment.blend.colorOperation));
        target->setSourceAlphaBlendFactor(blendFactor(attachment.blend.sourceAlpha));
        target->setDestinationAlphaBlendFactor(blendFactor(attachment.blend.destinationAlpha));
        target->setAlphaBlendOperation(blendOperation(attachment.blend.alphaOperation));
        target->setWriteMask(colorWriteMask(attachment.blend.writeMask));
    }

    if (description.depthFormat != PixelFormat::Undefined)
    {
        descriptor->setDepthAttachmentPixelFormat(toMetalPixelFormat(description.depthFormat));
        if (hasStencil(description.depthFormat))
        {
            descriptor->setStencilAttachmentPixelFormat(toMetalPixelFormat(description.depthFormat));
        }
    }

    if (!description.vertexBufferDescriptions.empty())
    {
        MTL::VertexDescriptor* vtxDesc = metal::VertexDescriptor::buildMetalVertexDescriptor(
            description.vertexBufferDescriptions);
        descriptor->setVertexDescriptor(vtxDesc);
        vtxDesc->release();
    }

    NS::Error* error = nullptr;
    _renderPipelineState = device->newRenderPipelineState(descriptor, &error);
    descriptor->release();

    if (!_renderPipelineState)
    {
        std::string errorMsg = "metal::GraphicsPipeline: failed to create render pipeline state";
        if (error)
        {
            errorMsg += " - " + std::string(error->localizedDescription()->utf8String());
        }
        throw std::runtime_error(errorMsg);
    }

    // When a depth attachment is present, build a depth-stencil state so depth
    // testing/writing is actually applied (the render pipeline state only stores
    // the attachment format). Mirrors the Vulkan default (test+write, Less).
    if (description.depthFormat != PixelFormat::Undefined)
    {
        auto* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
        const auto state = description.depthState.value_or(DepthState{true, true, CompareOperation::Less});
        depthDesc->setDepthCompareFunction(state.testEnabled ? compareOperation(state.compare) : MTL::CompareFunctionAlways);
        depthDesc->setDepthWriteEnabled(state.writeEnabled);
        _depthStencilState = device->newDepthStencilState(depthDesc);
        depthDesc->release();
        if (!_depthStencilState)
        {
            cleanup();
            throw std::runtime_error("Metal depth state creation failed");
        }
    }
}

GraphicsPipeline::~GraphicsPipeline()
{
    cleanup();
}

bool GraphicsPipeline::isValid() const
{
    return _renderPipelineState != nullptr;
}

void GraphicsPipeline::cleanup()
{
    if (_depthStencilState)
    {
        _depthStencilState->release();
        _depthStencilState = nullptr;
    }
    if (_renderPipelineState)
    {
        _renderPipelineState->release();
        _renderPipelineState = nullptr;
    }
}

#else

GraphicsPipeline::GraphicsPipeline(gpu::Device* gpuDevice, void* /*device*/, const gpu::GraphicsPipelineDescription& description)
    : gpu::GraphicsPipeline(gpuDevice), _topology(description.topology), _layout(dynamic_cast<metal::PipelineLayout*>(description.layout)), _cullMode(description.cullMode), _frontFace(description.frontFace)
{
}

GraphicsPipeline::~GraphicsPipeline()
{
    cleanup();
}

bool GraphicsPipeline::isValid() const
{
    return false;
}

void GraphicsPipeline::cleanup()
{
}

#endif

}
}
}
