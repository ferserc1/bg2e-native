#pragma once

#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Image.hpp>
#include <array>
#include <cmath>
#include <stdexcept>

namespace bg2e::gpu::tests {

// Manual primitive probe: supply modules built from the adjacent paired shaders.
// Run against an initialized device outside any frame/recording operation.
// It creates no scene, draw engine or production render object.
inline void runMrtProbe(Device& device, ShaderModule& vertex, ShaderModule& fragment)
{
    constexpr uint32_t attachmentCount = 6;
    const Size2D extent{16, 16};
    std::array<std::shared_ptr<Image>, attachmentCount> colors;
    for (auto& image : colors)
    {
        image = device.createImage({
            .size = extent,
            .format = PixelFormat::R8G8B8A8_UNORM,
            .usage = ImageUsage::ColorAttachment | ImageUsage::TransferSrc});
    }
    auto sampledOnly = device.createImage({
        .size = extent,
        .format = PixelFormat::R8G8B8A8_UNORM,
        .usage = ImageUsage::Sampled});
    auto depth = device.createImage({
        .size = extent,
        .format = PixelFormat::D32_SFLOAT,
        .usage = ImageUsage::DepthStencil});
    PipelineLayoutDescription layoutDescription;
    layoutDescription.pushConstants = {{0, sizeof(float), ShaderStage::Vertex},
                                       {sizeof(float), sizeof(float), ShaderStage::Fragment}};
    auto layout = device.createPipelineLayout(layoutDescription);
    GraphicsPipelineDescription pipelineDescription;
    pipelineDescription.vertexShader = &vertex;
    pipelineDescription.fragmentShader = &fragment;
    pipelineDescription.layout = layout.get();
    pipelineDescription.depthFormat = PixelFormat::D32_SFLOAT;
    pipelineDescription.depthState = DepthState{true, true, CompareOperation::Less};
    pipelineDescription.cullMode = CullMode::None;
    pipelineDescription.colorAttachments.resize(attachmentCount);
    for (auto& attachment : pipelineDescription.colorAttachments)
    {
        attachment.format = PixelFormat::R8G8B8A8_UNORM;
    }
    auto pipeline = device.createGraphicsPipeline(pipelineDescription);
    pipelineDescription.depthState->writeEnabled = false;
    auto readOnlyDepthPipeline = device.createGraphicsPipeline(pipelineDescription);

    // Validation failures must happen before any native render scope is opened.
    const auto expectRejected = [](auto&& operation)
    {
        try
        {
            operation();
        }
        catch (const std::invalid_argument&)
        {
            return;
        }
        throw std::runtime_error("MRT probe: incompatible configuration was accepted");
    };
    auto invalidPipeline = pipelineDescription;
    invalidPipeline.colorAttachments.resize(device.renderingCapabilities().maxColorAttachments + 1);
    expectRejected([&]()
    {
        GraphicsPipeline::validateDescription(invalidPipeline, device);
    });

    const auto record = [&](CommandBuffer& command, GraphicsPipeline& selected,
                            float z, float multiplier, bool clear)
    {
        RenderingDescription rendering;
        for (const auto& image : colors)
        {
            command.transition(image.get(), ImageLayout::ColorAttachment);
            ColorAttachmentDescription attachment;
            attachment.image = image.get();
            attachment.load = clear ? AttachmentLoadOperation::Clear : AttachmentLoadOperation::Load;
            attachment.clear.floatingPoint = {0, 0, 0, 1};
            rendering.colors.push_back(attachment);
        }
        command.transition(depth.get(), ImageLayout::DepthAttachment);
        rendering.depth = DepthAttachmentDescription{
            depth.get(), clear ? AttachmentLoadOperation::Clear : AttachmentLoadOperation::Load,
            AttachmentStoreOperation::Store, 1.0f, 0};
        auto wrongFormat = rendering;
        wrongFormat.colors[0].image = depth.get();
        expectRejected([&]()
        {
            CommandBuffer::validateRendering(wrongFormat, device);
        });
        auto wrongUsage = rendering;
        wrongUsage.colors[0].image = sampledOnly.get();
        expectRejected([&]()
        {
            CommandBuffer::validateRendering(wrongUsage, device);
        });
        auto wrongCount = rendering;
        wrongCount.colors.pop_back();
        expectRejected([&]()
        {
            CommandBuffer::validatePipelineAttachments(wrongCount, selected);
        });
        command.beginRendering(rendering);
        command.bindPipeline(&selected);
        command.pushConstants(ShaderStage::Vertex, 0, sizeof(z), &z);
        command.pushConstants(ShaderStage::Fragment, sizeof(float), sizeof(multiplier), &multiplier);
        command.draw(3, 1, 0, 0);
        command.endRendering();
    };
    const auto verify = [&](float multiplier)
    {
        const std::array<std::array<uint8_t, 3>, attachmentCount> expected = {{{255, 0, 0}, {0, 255, 0},
            {0, 0, 255}, {255, 255, 0}, {255, 0, 255}, {0, 255, 255}}};
        for (size_t i = 0; i < colors.size(); ++i)
        {
            std::vector<uint8_t> pixels;
            colors[i]->readPixelsRGBA8(pixels, ImageLayout::ColorAttachment);
            if (pixels.size() != extent.width * extent.height * 4)
            {
                throw std::runtime_error("MRT probe: invalid readback size");
            }
            for (size_t pixel = 0; pixel < pixels.size(); pixel += 4)
            {
                for (size_t channel = 0; channel < 3; ++channel)
                {
                    const int value = static_cast<int>(std::lround(expected[i][channel] * multiplier));
                    if (std::abs(static_cast<int>(pixels[pixel + channel]) - value) > 1)
                    {
                        throw std::runtime_error("MRT probe: color routing/depth persistence mismatch");
                    }
                }
                if (pixels[pixel + 3] != 255)
                {
                    throw std::runtime_error("MRT probe: invalid alpha channel");
                }
            }
        }
    };
    device.immediateSubmit([&](CommandBuffer* command)
    {
        record(*command, *pipeline, 0.25f, 1.0f, true);
        // Far geometry writes black if stored depth was lost or ignored.
        record(*command, *pipeline, 0.75f, 0.0f, false);
    });
    verify(1.0f);
    device.immediateSubmit([&](CommandBuffer* command)
    {
        // Read-only depth must not replace 0.25 with 0.1. The next draw at
        // 0.2 passes only if independent depth-write disable was honored.
        record(*command, *readOnlyDepthPipeline, 0.1f, 0.5f, false);
        record(*command, *pipeline, 0.2f, 0.25f, false);
    });
    verify(0.25f);
}

}
