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
#include <bg2e/base/Log.hpp>

#include <stdexcept>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

MTL::PrimitiveType primitiveTopologyToMetal(gpu::PrimitiveTopology topology)
{
    switch (topology)
    {
        case gpu::PrimitiveTopology::TriangleList:  return MTL::PrimitiveTypeTriangle;
        case gpu::PrimitiveTopology::TriangleStrip: return MTL::PrimitiveTypeTriangleStrip;
        case gpu::PrimitiveTopology::LineList:      return MTL::PrimitiveTypeLine;
        case gpu::PrimitiveTopology::PointList:     return MTL::PrimitiveTypePoint;
    }
    return MTL::PrimitiveTypeTriangle;
}

GraphicsPipeline::GraphicsPipeline(MTL::Device* device, const gpu::GraphicsPipelineDescription& description)
    : _topology(description.topology)
    , _layout(dynamic_cast<metal::PipelineLayout*>(description.layout))
{
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

    descriptor->colorAttachments()->object(0)->setPixelFormat(toMetalPixelFormat(description.colorFormat));

    if (description.depthFormat != PixelFormat::Undefined)
    {
        descriptor->setDepthAttachmentPixelFormat(toMetalPixelFormat(description.depthFormat));
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
    if (_renderPipelineState)
    {
        _renderPipelineState->release();
        _renderPipelineState = nullptr;
    }
}

#else

GraphicsPipeline::GraphicsPipeline(void* /*device*/, const gpu::GraphicsPipelineDescription& description)
    : _topology(description.topology)
    , _layout(dynamic_cast<metal::PipelineLayout*>(description.layout))
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
