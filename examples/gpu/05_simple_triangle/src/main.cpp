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
#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <bg2e/app/SDLUtils.hpp>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

struct PushConstants {
    float color[4];
};

int main(int argc, char** argv)
{
    using namespace bg2e;

    // 1. Select backend per platform
    auto backendType = gpu::BackendType::Vulkan;
    if (base::PlatformTools::currentPlatform() == base::Platform::macOS)
    {
        std::cout << "Select backend [1=Metal, 2=Vulkan]: ";
        int choice = 0;
        std::cin >> choice;
        backendType = (choice == 2) ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
    }

    // 2. Init backend to query window type
    gpu::Factory::init(backendType);
    auto* backend = gpu::Factory::backend();

    // 3. SDL init + create window based on backend window type
    app::initSdlVideoDriver();
    SDL_Init(SDL_INIT_VIDEO);

    Uint32 windowFlags = 0;
    switch (backend->windowType())
    {
        case gpu::WindowType::Vulkan: windowFlags = SDL_WINDOW_VULKAN; break;
        case gpu::WindowType::Metal:  windowFlags = SDL_WINDOW_METAL;  break;
    }

    SDL_Window* window = SDL_CreateWindow(
        "GPU Simple Triangle Example",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        windowFlags
    );
    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 4. Create GPU instance
    auto* instance = backend->sharedInstance();
    instance->enableDebugMode(true);
    instance->create(window);

    // 5. Create surface (shared_ptr for the frame lifecycle API)
    std::shared_ptr<gpu::Surface> surface = backend->createWindowSurface(instance);

    // 6. Select physical device
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    // 7. Create logical device
    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());

    // 8. Create shader modules and pipeline
    auto shaderBasePath = base::PlatformTools::shaderPath();
    std::string targetName = "gpu_simple_triangle";

    std::unique_ptr<gpu::ShaderModule> vs;
    std::unique_ptr<gpu::ShaderModule> fs;
    std::unique_ptr<gpu::ShaderModule> cs;

    if (backendType == gpu::BackendType::Vulkan)
    {
        auto vsPath = (shaderBasePath / targetName / "triangle.vert.spv").string();
        auto fsPath = (shaderBasePath / targetName / "triangle.frag.spv").string();
        auto csPath = (shaderBasePath / targetName / "gradient.comp.spv").string();
        vs = device->createShaderModule({ vsPath, "main", gpu::ShaderStage::Vertex, "Triangle vertex shader" });
        fs = device->createShaderModule({ fsPath, "main", gpu::ShaderStage::Fragment, "Triangle fragment shader" });
        cs = device->createShaderModule({ csPath, "main", gpu::ShaderStage::Compute, "Gradient compute shader" });
    }
    else
    {
        auto libPath = (shaderBasePath / targetName / "metal" / "triangle.metallib").string();
        vs = device->createShaderModule({ libPath, "triangle_vertex", gpu::ShaderStage::Vertex, "Triangle vertex shader" });
        fs = device->createShaderModule({ libPath, "triangle_fragment", gpu::ShaderStage::Fragment, "Triangle fragment shader" });
        cs = device->createShaderModule({ libPath, "gradient_compute", gpu::ShaderStage::Compute, "Gradient compute shader" });
    }

    // Graphics layout with push constant range and SampledImage + Sampler bindings
    gpu::PipelineLayoutDescription graphicsLayoutDesc{};
    graphicsLayoutDesc.pushConstants.push_back({ 0, sizeof(PushConstants), gpu::ShaderStage::Fragment });
    graphicsLayoutDesc.resourceBindings.push_back({ 0, 0, gpu::ResourceType::SampledImage, gpu::ShaderStage::Fragment, 1 });
    graphicsLayoutDesc.resourceBindings.push_back({ 0, 1, gpu::ResourceType::Sampler,      gpu::ShaderStage::Fragment, 1 });
    graphicsLayoutDesc.debugName = "Graphics pipeline layout";
    auto graphicsLayout = device->createPipelineLayout(graphicsLayoutDesc);

    // Procedural 2×2 texture
    const std::array<std::array<uint8_t,4>, 4> texels = {{
        {{255,   0,   0, 255}}, {{  0, 255,   0, 255}},
        {{  0,   0, 255, 255}}, {{255, 255,   0, 255}}
    }};
    auto texture = device->createImage({ {2, 2}, gpu::PixelFormat::R8G8B8A8_UNORM,
                                          gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst,
                                          "Procedural 2x2 texture" });
    texture->uploadRGBA8(texels.data(), { 2, 2 });
    device->immediateSubmit([texture](gpu::CommandBuffer* cmd)
    {
        cmd->transition(texture.get(), gpu::ImageLayout::ShaderReadOnly);
    });

    // Sampler with default settings (linear/linear, repeat)
    auto sampler = device->createSampler({ .debugName = "Default linear sampler" });

    // Static graphics resource set (texture + sampler, bound every frame)
    auto textureSet = device->createResourceSet(graphicsLayout.get(), 0, "Static texture set");
    textureSet->setSampledImage(0, texture.get());
    textureSet->setSampler(1, sampler.get());
    textureSet->update();

    bg2e::base::Log::isDebug();
    // Compute layout with StorageImage binding at set=0, binding=0
    gpu::PipelineLayoutDescription computeLayoutDesc{};
    computeLayoutDesc.resourceBindings.push_back({
        0,                                  // set
        0,                                  // binding
        gpu::ResourceType::StorageImage,
        gpu::ShaderStage::Compute,
        1                                   // count
    });
    computeLayoutDesc.debugName = "Compute pipeline layout";
    auto computeLayout = device->createPipelineLayout(computeLayoutDesc);

    // Get attachment formats from the first frame
    auto colorFormat = surface->colorFormat();
    auto depthFormat = surface->depthFormat();

    gpu::GraphicsPipelineDescription pipelineDesc{};
    pipelineDesc.vertexShader = vs.get();
    pipelineDesc.fragmentShader = fs.get();
    pipelineDesc.layout = graphicsLayout.get();
    pipelineDesc.topology = gpu::PrimitiveTopology::TriangleList;
    pipelineDesc.colorFormat = colorFormat;
    pipelineDesc.depthFormat = depthFormat;
    pipelineDesc.debugName = "Main graphics pipeline";

    auto pipeline = device->createGraphicsPipeline(pipelineDesc);

    // Compute pipeline
    gpu::ComputePipelineDescription computePipelineDesc{};
    computePipelineDesc.computeShader = cs.get();
    computePipelineDesc.layout = computeLayout.get();
    computePipelineDesc.debugName = "Gradient compute pipeline";
    auto computePipeline = device->createComputePipeline(computePipelineDesc);

    // Resource set ring: one set per swapchain image
    auto imageCount = surface->imageCount();
    std::vector<std::unique_ptr<gpu::ResourceSet>> resourceSets;
    resourceSets.reserve(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i)
    {
        resourceSets.push_back(device->createResourceSet(computeLayout.get(), 0,
            "Compute resource set ring[" + std::to_string(i) + "]"));
    }
    uint32_t ringIndex = 0;

    // 9. Render loop
    auto& graphicsQueue = device->graphicsQueue();

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE) running = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                surface->resize({ static_cast<uint32_t>(event.window.data1),
                                  static_cast<uint32_t>(event.window.data2) });
            }
        }

        const float t = static_cast<float>(SDL_GetTicks64()) / 1000.0f;

        PushConstants push{};
        push.color[0] = 0.5f + 0.5f * std::sin(t * 1.3f);
        push.color[1] = 0.5f + 0.5f * std::sin(t * 1.7f + 1.0f);
        push.color[2] = 0.5f + 0.5f * std::sin(t * 2.1f + 2.0f);
        push.color[3] = 1.0f;

        auto frame = surface->beginFrame();
        auto cmd   = graphicsQueue.createCommandBuffer("Frame command buffer");

        cmd->begin();

        // Transition color image to General for compute write
        cmd->transition(frame->colorImage(), gpu::ImageLayout::General);

        // Compute dispatch: write gradient into the color image
        cmd->beginCompute();
        cmd->bindPipeline(computePipeline.get());

        auto* rs = resourceSets[ringIndex].get();
        rs->setStorageImage(0, frame->colorImage());
        rs->update();
        cmd->bindResourceSet(computePipeline.get(), 0, rs);

        // Both backends now use 16x16 threads-per-group, so the same
        // group-count calculation works for Vulkan and Metal.
        uint32_t gx = (frame->colorImage()->width()  + 15) / 16;
        uint32_t gy = (frame->colorImage()->height() + 15) / 16;
        cmd->dispatch(gx, gy, 1);

        cmd->endCompute();

        // Graphics rendering (gradient is the background, no clearColor)
        cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
        cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
        cmd->beginRendering(frame.get());
        cmd->clearDepth(1.0f);
        cmd->bindPipeline(pipeline.get());
        cmd->bindResourceSet(pipeline.get(), 0, textureSet.get());
        cmd->pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(PushConstants), &push);
        cmd->draw(3);
        cmd->endRendering();
        cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

        surface->present(cmd.get());
        cmd->end();
        graphicsQueue.submit(cmd.get());
        surface->endFrame(frame.get());

        ringIndex = (ringIndex + 1) % imageCount;
    }

    // 10. Cleanup
    device->waitIdle();
    for (auto& rs : resourceSets)
    {
        rs->cleanup();
    }
    computePipeline->cleanup();
    pipeline->cleanup();
    textureSet->cleanup();
    sampler.reset();
    texture.reset();
    computeLayout->cleanup();
    graphicsLayout->cleanup();
    cs->cleanup();
    vs->cleanup();
    fs->cleanup();
    surface->cleanup();
    device->cleanup();
    instance->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
