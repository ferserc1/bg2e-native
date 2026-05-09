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

#include <bg2e/render/deferred/RenderLayer.hpp>

namespace bg2e::render::deferred {

RenderLayer::RenderLayer(Engine* engine)
    : _engine { engine }
    , _extent { 0, 0 }
    , _outputFormat { VK_FORMAT_UNDEFINED }
{
}

RenderLayer::~RenderLayer()
{
}

void RenderLayer::build(VkExtent2D extent, VkFormat outputFormat)
{
    _extent = extent;
    _outputFormat = outputFormat;
}

void RenderLayer::initFrameResources(vulkan::DescriptorSetAllocator*)
{
}

void RenderLayer::render(
    VkCommandBuffer,
    uint32_t,
    const vulkan::Image*,
    const vulkan::Image*,
    vulkan::FrameResources&
)
{
}

void RenderLayer::resize(VkExtent2D newExtent)
{
    _extent = newExtent;
}

void RenderLayer::cleanup()
{
}

}
