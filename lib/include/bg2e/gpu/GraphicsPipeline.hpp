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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>
#include <bg2e/gpu/DeviceResource.hpp>
#include <bg2e/gpu/ShaderModule.hpp>
#include <bg2e/gpu/PipelineLayout.hpp>

namespace bg2e {
namespace gpu {

enum class PrimitiveTopology
{
    TriangleList,
    TriangleStrip,
    LineList,
    PointList
};

struct GraphicsPipelineDescription {
    gpu::ShaderModule* vertexShader = nullptr;   // non-owning; caller must keep alive
    gpu::ShaderModule* fragmentShader = nullptr; // non-owning; caller must keep alive
    gpu::PipelineLayout* layout = nullptr;       // non-owning; caller must keep alive
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    PixelFormat colorFormat = PixelFormat::Undefined; // from surface frame
    PixelFormat depthFormat = PixelFormat::Undefined; // Undefined => no depth
    CullMode cullMode = CullMode::Back;
    FrontFace frontFace = FrontFace::CounterClockwise;
    std::string debugName; // Debug label for validation layers / Xcode GPU Capture

    std::vector<VertexBufferDescription> vertexBufferDescriptions;

    // Nonempty array replaces legacy colorFormat; conflicting declarations fail.
    // Empty array + Undefined colorFormat represents a depth-only pipeline.
    std::vector<ColorAttachmentState> colorAttachments;
    // Unset preserves each backend's historical depth defaults.
    std::optional<DepthState> depthState;

    std::vector<ColorAttachmentState> resolvedColorAttachments() const
    {
        if (!colorAttachments.empty())
        {
            if (colorFormat != PixelFormat::Undefined)
            {
                throw std::invalid_argument("Use colorAttachments or colorFormat, not both");
            }
            return colorAttachments;
        }
        if (colorFormat != PixelFormat::Undefined)
        {
            return {{colorFormat, {}}};
        }
        return {};
    }

    void addVertexBufferDescription(const VertexBufferDescription& desc)
    {
        vertexBufferDescriptions.push_back(desc);
    }
};

class BG2E_API GraphicsPipeline : public DeviceResource {
  public:
    explicit GraphicsPipeline(Device* device) : DeviceResource(device)
    {
    }
    virtual ~GraphicsPipeline() = default;

    const std::vector<ColorAttachmentState>& colorAttachments() const
    {
        return _colorAttachments;
    }
    PixelFormat depthFormat() const
    {
        return _depthFormat;
    }
    static void validateDescription(const GraphicsPipelineDescription& description, const Device& device);

  protected:
    void setAttachmentDescription(const GraphicsPipelineDescription& description)
    {
        _colorAttachments = description.resolvedColorAttachments();
        _depthFormat = description.depthFormat;
    }
    std::vector<ColorAttachmentState> _colorAttachments;
    PixelFormat _depthFormat = PixelFormat::Undefined;
};

}
}
