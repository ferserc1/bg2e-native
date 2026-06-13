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

#include <bg2e/gpu/metal/VertexDescriptor.hpp>

namespace bg2e {
namespace gpu {
namespace metal {

#if BG2E_IS_MAC

MTL::VertexFormat VertexDescriptor::toMetalVertexFormat(gpu::Format format)
{
    switch (format)
    {
        case gpu::Format::R32_SFLOAT:          return MTL::VertexFormatFloat;
        case gpu::Format::R32G32_SFLOAT:       return MTL::VertexFormatFloat2;
        case gpu::Format::R32G32B32_SFLOAT:    return MTL::VertexFormatFloat3;
        case gpu::Format::R32G32B32A32_SFLOAT: return MTL::VertexFormatFloat4;
        default:                               return MTL::VertexFormatInvalid;
    }
}

MTL::VertexDescriptor* VertexDescriptor::buildMetalVertexDescriptor(
    const std::vector<gpu::VertexBufferDescription>& descriptions)
{
    auto* vtxDesc = MTL::VertexDescriptor::alloc()->init();

    for (const auto& desc : descriptions)
    {
        auto* layout = vtxDesc->layouts()->object(desc.binding);
        layout->setStride(desc.stride);
        layout->setStepFunction(desc.inputRate == gpu::VertexInputRate::Vertex
            ? MTL::VertexStepFunctionPerVertex
            : MTL::VertexStepFunctionPerInstance);
        layout->setStepRate(1);

        for (const auto& attr : desc.attributes)
        {
            auto* attribute = vtxDesc->attributes()->object(attr.location);
            attribute->setFormat(toMetalVertexFormat(attr.format));
            attribute->setOffset(attr.offset);
            attribute->setBufferIndex(attr.binding);
        }
    }

    return vtxDesc;
}

#endif

}
}
}
