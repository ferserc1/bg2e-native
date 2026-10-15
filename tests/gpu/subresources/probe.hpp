#pragma once

#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <array>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

namespace bg2e::gpu::tests {

inline void runSubresourceProbe(Device& device)
{
    const auto expectRejected = [](auto&& operation)
    {
        try
        {
            operation();
        }
        catch (const std::logic_error&)
        {
            return;
        }
        throw std::runtime_error("Subresource probe: invalid operation was accepted");
    };
    const auto checkRgba = [](Image& image, const ImageSubresourceRange& range, const std::array<uint8_t, 4>& expected)
    {
        std::vector<uint8_t> pixels;
        image.readPixels(pixels, range);
        const auto extent = image.mipExtent(range.baseMipLevel);
        if (pixels.size() != size_t(extent.width) * extent.height * 4)
        {
            throw std::runtime_error("Subresource probe: readback extent mismatch");
        }
        for (size_t pixel = 0; pixel < pixels.size(); pixel += 4)
        {
            for (size_t channel = 0; channel < 4; ++channel)
            {
                if (std::abs(int(pixels[pixel + channel]) - int(expected[channel])) > 1)
                {
                    throw std::runtime_error("Subresource probe: texels or mip/layer selection differ");
                }
            }
        }
    };
    const auto usages = ImageUsage::ColorAttachment | ImageUsage::Sampled | ImageUsage::TransferSrc | ImageUsage::TransferDst;
    auto texture = device.createImage({.size = {7, 5}, .format = PixelFormat::R8G8B8A8_UNORM,
        .usage = usages, .minMipSize = 1});
    const std::array<uint8_t, 4> baseColor{64, 128, 192, 255};
    std::vector<uint8_t> basePixels(7 * 5 * 4);
    for (size_t pixel = 0; pixel < basePixels.size(); pixel += 4)
    {
        std::memcpy(basePixels.data() + pixel, baseColor.data(), 4);
    }
    std::vector<uint8_t> paddedRows(32 * 5);
    for (size_t row = 0; row < 5; ++row)
    {
        std::memcpy(paddedRows.data() + row * 32, basePixels.data() + row * 28, 28);
    }
    texture->upload({paddedRows.data(), paddedRows.size(), 32});
    if (texture->mipLevels() != 3 || texture->mipExtent(1) != Size2D{3, 2} || texture->mipExtent(2) != Size2D{1, 1} ||
        texture->currentLayout({0, 1, 0, 1}) != ImageLayout::ShaderReadOnly ||
        texture->currentLayout({1, 1, 0, 1}) != ImageLayout::Undefined)
    {
        throw std::runtime_error("Subresource probe: allocation/initial layout mismatch");
    }
    expectRejected([&]()
    {
        texture->currentLayout(); // Mixed ranges cannot be mistaken for Undefined.
    });
    texture->generateMipmaps();
    for (uint32_t mip = 0; mip < texture->mipLevels(); ++mip)
    {
        checkRgba(*texture, {mip, 1, 0, 1}, baseColor);
    }
    // The convenience overload must target mip one rather than silently zero.
    device.immediateSubmit([&](CommandBuffer* command)
    {
        command->transition(texture.get(), ImageLayout::ColorAttachment, {1, 1, 0, 1});
        command->beginRendering(texture.get(), 1);
        command->clearColor(0, {1, 0, 0, 1});
        command->endRendering();
        command->transition(texture.get(), ImageLayout::ShaderReadOnly, {1, 1, 0, 1});
    });
    checkRgba(*texture, {0, 1, 0, 1}, baseColor);
    checkRgba(*texture, {1, 1, 0, 1}, {255, 0, 0, 255});
    checkRgba(*texture, {2, 1, 0, 1}, baseColor);

    auto copied = device.createImage({.size = {3, 2}, .format = PixelFormat::R8G8B8A8_UNORM, .usage = usages});
    device.immediateSubmit([&](CommandBuffer* command)
    {
        command->transition(texture.get(), ImageLayout::TransferSrc, {1, 1, 0, 1});
        command->transition(copied.get(), ImageLayout::TransferDst);
        command->copyImage(texture->view({1, 1, 0, 1}), copied->fullView());
        command->transition(texture.get(), ImageLayout::ShaderReadOnly, {1, 1, 0, 1});
        command->transition(copied.get(), ImageLayout::ShaderReadOnly);
    });
    checkRgba(*copied, {}, {255, 0, 0, 255});

    const std::array<std::array<uint8_t, 4>, 6> faces = {{{255, 0, 0, 255}, {0, 255, 0, 255},
        {0, 0, 255, 255}, {255, 255, 0, 255}, {255, 0, 255, 255}, {0, 255, 255, 255}}};
    auto cube = device.createCubeMap({.size = 8, .format = PixelFormat::R8G8B8A8_UNORM,
        .usage = usages, .minMipSize = 1});
    device.immediateSubmit([&](CommandBuffer* command)
    {
        for (uint32_t face = 0; face < faces.size(); ++face)
        {
            const ImageSubresourceRange range{0, 1, face, 1};
            command->transition(cube->image(), ImageLayout::ColorAttachment, range);
            command->beginRendering(cube.get(), static_cast<CubemapFace>(face));
            command->clearColor(0, {faces[face][0] / 255.0f, faces[face][1] / 255.0f, faces[face][2] / 255.0f, 1});
            command->endRendering();
            command->transition(cube->image(), ImageLayout::ShaderReadOnly, range);
        }
        command->generateMipmaps(cube->image());
    });
    for (uint32_t face = 0; face < faces.size(); ++face)
    {
        for (uint32_t mip = 0; mip < cube->mipLevels(); ++mip)
        {
            checkRgba(*cube->image(), {mip, 1, face, 1}, faces[face]);
        }
    }

    // Exercise native sampled views without changing ShaderBinding's indices.
    PipelineLayoutDescription viewLayoutDescription;
    viewLayoutDescription.resourceBindings = {
        {0, {.vulkan = 0, .metal = 0}, ResourceType::SampledImage, ShaderStage::Fragment, 1},
        {0, {.vulkan = 1, .metal = 1}, ResourceType::SampledImage, ShaderStage::Fragment, 1}};
    auto viewLayout = device.createPipelineLayout(viewLayoutDescription);
    auto viewSet = device.createResourceSet(viewLayout.get(), 0);
    viewSet->setSampledImage({.vulkan = 0, .metal = 0}, texture->view({1, 1, 0, 1}));
    viewSet->setSampledImage({.vulkan = 1, .metal = 1}, cube->image()->view({1, 1, 0, 6}));
    viewSet->update();

    // Packed HDR uploads/readback must preserve out-of-range float components.
    auto hdr = device.createImage({.size = {1, 1}, .format = PixelFormat::R32G32B32A32_SFLOAT,
        .usage = ImageUsage::Sampled | ImageUsage::TransferSrc | ImageUsage::TransferDst});
    const std::array<float, 4> hdrPixels{4.0f, -0.5f, 0.25f, 1.0f};
    hdr->upload({hdrPixels.data(), sizeof(hdrPixels)});
    hdr->generateMipmaps(); // Single-level final transition, no filtering required.
    std::vector<uint8_t> hdrReadback;
    hdr->readPixels(hdrReadback, {});
    if (hdrReadback.size() != sizeof(hdrPixels) || std::memcmp(hdrReadback.data(), hdrPixels.data(), sizeof(hdrPixels)))
    {
        throw std::runtime_error("Subresource probe: HDR components were changed");
    }
    expectRejected([&]()
    {
        texture->view({texture->mipLevels(), 1, 0, 1});
    });
    expectRejected([&]()
    {
        texture->upload({basePixels.data(), 1});
    });
    expectRejected([&]()
    {
        device.createImage({.size = {3, 1}, .mipLevels = 3});
    });
    auto integer = device.createImage({.size = {2, 2}, .format = PixelFormat::R32_UINT,
        .usage = ImageUsage::Sampled | ImageUsage::TransferSrc | ImageUsage::TransferDst, .mipLevels = 2});
    const std::array<uint32_t, 4> values{1, 2, 3, 4};
    integer->upload({values.data(), sizeof(values)});
    expectRejected([&]()
    {
        integer->generateMipmaps();
    });
}

}
