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
#include <cmath>
#include <iostream>

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

    if (backendType == gpu::BackendType::Vulkan)
    {
        auto vsPath = (shaderBasePath / targetName / "triangle.vert.spv").string();
        auto fsPath = (shaderBasePath / targetName / "triangle.frag.spv").string();
        vs = device->createShaderModule({ vsPath, "main", gpu::ShaderStage::Vertex });
        fs = device->createShaderModule({ fsPath, "main", gpu::ShaderStage::Fragment });
    }
    else
    {
        auto libPath = (shaderBasePath / targetName / "metal" / "triangle.metallib").string();
        vs = device->createShaderModule({ libPath, "triangle_vertex", gpu::ShaderStage::Vertex });
        fs = device->createShaderModule({ libPath, "triangle_fragment", gpu::ShaderStage::Fragment });
    }

    auto layout = device->createPipelineLayout({});

    // Get attachment formats from the first frame
    auto colorFormat = surface->colorFormat();
    auto depthFormat = surface->depthFormat();

    gpu::GraphicsPipelineDescription pipelineDesc{};
    pipelineDesc.vertexShader = vs.get();
    pipelineDesc.fragmentShader = fs.get();
    pipelineDesc.layout = layout.get();
    pipelineDesc.topology = gpu::PrimitiveTopology::TriangleList;
    pipelineDesc.colorFormat = colorFormat;
    pipelineDesc.depthFormat = depthFormat;

    auto pipeline = device->createGraphicsPipeline(pipelineDesc);

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
        gpu::Color clearColor {
            0.5f + 0.5f * std::sin(t),
            0.5f + 0.5f * std::sin(t + 2.0f),
            0.5f + 0.5f * std::sin(t + 4.0f),
            1.0f
        };

        auto frame = surface->beginFrame();
        auto cmd   = graphicsQueue.createCommandBuffer();

        cmd->begin();
        cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
        cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
        cmd->beginRendering(frame.get());
        cmd->clearColor(0, clearColor);
        cmd->clearDepth(1.0f);
        cmd->bindPipeline(pipeline.get());
        cmd->draw(3);
        cmd->endRendering();
        cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

        surface->present(cmd.get());
        cmd->end();
        graphicsQueue.submit(cmd.get());
        surface->endFrame(frame.get());
    }

    // 10. Cleanup
    device->waitIdle();
    pipeline->cleanup();
    layout->cleanup();
    vs->cleanup();
    fs->cleanup();
    surface->cleanup();
    device->cleanup();
    instance->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
