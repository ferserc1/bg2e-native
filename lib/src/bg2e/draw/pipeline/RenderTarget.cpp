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

#include <bg2e/draw/pipeline/RenderTarget.hpp>
#include <bg2e/gpu/Device.hpp>

namespace bg2e::draw::pipeline {
std::shared_ptr<gpu::Image> RenderTarget::create(Engine& engine, gpu::Size2D extent,
    gpu::PixelFormat format, const std::string& name)
{
    gpu::ImageDescription description;
    description.size = extent;
    description.format = format;
    description.debugName = name;
    description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst |
        (gpu::isDepthFormat(format) ? gpu::ImageUsage::DepthStencil : gpu::ImageUsage::ColorAttachment);
    const auto supported = engine.device()->formatCapabilities(format).usages;
    if (extent.isZero() || (supported & description.usage) != description.usage)
    {
        throw std::invalid_argument("Render target extent/format usages are unsupported");
    }
    return engine.device()->createImage(description);
}

void RenderTarget::clear(gpu::CommandBuffer& command, gpu::Image& color, gpu::Color value)
{
    command.transition(&color, gpu::ImageLayout::ColorAttachment);
    gpu::RenderingDescription rendering;
    gpu::ColorAttachmentDescription attachment;
    attachment.image = &color;
    attachment.load = gpu::AttachmentLoadOperation::Clear;
    attachment.clear.floatingPoint = value;
    rendering.colors.push_back(attachment);
    command.beginRendering(rendering);
    command.endRendering();
    command.transition(&color, gpu::ImageLayout::ShaderReadOnly);
}

void RenderTarget::copy(gpu::CommandBuffer& command, gpu::Image& source, gpu::Image& destination)
{
    command.transition(&source, gpu::ImageLayout::TransferSrc);
    command.transition(&destination, gpu::ImageLayout::TransferDst);
    command.copyImage(source.view({0, 1, 0, 1}), destination.view({0, 1, 0, 1}));
    command.transition(&source, gpu::ImageLayout::ShaderReadOnly);
    command.transition(&destination, gpu::ImageLayout::ShaderReadOnly);
}
}
