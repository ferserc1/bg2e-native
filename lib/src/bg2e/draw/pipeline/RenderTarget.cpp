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
