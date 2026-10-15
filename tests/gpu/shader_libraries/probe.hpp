#pragma once

#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/ResourceSet.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <cmath>
#include <stdexcept>

namespace bg2e::gpu::tests {

// Use the deployed engine shaders directory, including inside installed bundles.
inline void runShaderLibraryProbe(Device& device, BackendType backend, const std::filesystem::path& root)
{
    const auto verify = [](Image& image, ImageLayout layout, float expected)
    {
        std::vector<uint8_t> pixels;
        image.readPixelsRGBA8(pixels, layout);
        if (pixels.size() != 7 * 5 * 4)
        {
            throw std::runtime_error("Shader library probe: unexpected readback size");
        }
        for (size_t i = 0; i < pixels.size(); i += 4)
        {
            for (size_t channel = 0; channel < 3; ++channel)
            {
                if (std::abs(int(pixels[i + channel]) - int(std::lround(expected * 255))) > 2)
                {
                    throw std::runtime_error("Shader library probe: shader output mismatch");
                }
            }
            if (pixels[i + 3] != 255)
            {
                throw std::runtime_error("Shader library probe: alpha mismatch");
            }
        }
    };
    auto image = device.createImage({.size = {7, 5}, .format = PixelFormat::R8G8B8A8_UNORM,
        .usage = ImageUsage::ColorAttachment | ImageUsage::Storage | ImageUsage::TransferSrc});
    auto layout = device.createPipelineLayout({});
    for (const auto* name : {"gbuffer", "composite"})
    {
        ShaderLib library(root / "draw" / name, backend);
        auto vertex = library.vertex("probe", &device);
        auto fragment = library.fragment("probe", &device);
        GraphicsPipelineDescription description;
        description.vertexShader = vertex.get();
        description.fragmentShader = fragment.get();
        description.layout = layout.get();
        description.colorFormat = PixelFormat::R8G8B8A8_UNORM;
        description.cullMode = CullMode::None;
        auto pipeline = device.createGraphicsPipeline(description);
        device.immediateSubmit([&](CommandBuffer* command)
        {
            command->transition(image.get(), ImageLayout::ColorAttachment);
            RenderingDescription rendering;
            rendering.colors.push_back({.image = image.get()});
            command->beginRendering(rendering);
            command->bindPipeline(pipeline.get());
            command->draw(3, 1, 0, 0);
            command->endRendering();
        });
        // Normal incidence BRDF: 0.96 * 0.5 / pi + 16 * 0.04 / (4*pi).
        const float expected = std::string(name) == "gbuffer"
            ? 0.64f / 3.14159265358979323846f : std::pow(1.0f - std::exp(-0.5f), 1.0f / 2.2f);
        verify(*image, ImageLayout::ColorAttachment, expected);
    }
    ShaderLib composite(root / "draw/composite", backend);
    auto compute = composite.compute("probe", &device);
    PipelineLayoutDescription computeLayoutDescription;
    computeLayoutDescription.resourceBindings.push_back({0, {0, 0}, ResourceType::StorageImage, ShaderStage::Compute});
    auto computeLayout = device.createPipelineLayout(computeLayoutDescription);
    auto pipeline = device.createComputePipeline({compute.get(), computeLayout.get()});
    auto resources = device.createResourceSet(computeLayout.get(), 0);
    resources->setStorageImage({0, 0}, image.get());
    resources->update();
    device.immediateSubmit([&](CommandBuffer* command)
    {
        command->transition(image.get(), ImageLayout::General);
        command->beginCompute();
        command->bindPipeline(pipeline.get());
        command->bindResourceSet(pipeline.get(), 0, resources.get());
        command->dispatch(1, 1, 1);
        command->endCompute();
    });
    verify(*image, ImageLayout::General, std::pow(1.0f - std::exp(-0.5f), 1.0f / 2.2f));
}

}
