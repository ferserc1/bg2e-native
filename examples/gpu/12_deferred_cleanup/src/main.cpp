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

// Example 12 — Deferred Cleanup Validation
//
// Demonstrates deferred resource cleanup by switching between cube and sphere
// geometry every 5 seconds. When switching, the old GPU buffers are scheduled
// for deferred destruction via CleanupManager::defer() rather than immediate
// deletion.
//
//   - gpu::CleanupManager with defer()/flushDeferred() for deferred execution
//     tied to the surface's frame counter.

#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <bg2e/app/SDLUtils.hpp>
#include <bg2e/math/base.hpp>

#include <array>
#include <iostream>
#include <memory>

struct CameraUBO {
    glm::mat4 projectionView;
};

struct ModelUBO {
    glm::mat4 model;
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
        "GPU Deferred Cleanup Example",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        windowFlags | SDL_WINDOW_RESIZABLE
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

    gpu::CleanupManager cleanup(surface.get());

    // 8. Create shader modules via ShaderLib
    auto shaderBasePath = base::PlatformTools::shaderPath();

    auto shaderLib = backend->createShaderLib(shaderBasePath / "gpu_deferred_cleanup");
    auto vs = shaderLib->vertex("cube", device.get());
    auto fs = shaderLib->fragment("cube", device.get());
    cleanup.push(vs);
    cleanup.push(fs);

    // 9. Pipeline layout with separate sets:
    //   set 0 binding 0 -> camera UBO  (vertex)
    //   set 1 binding 0 -> model UBO   (vertex)
    //   set 2 binding 0 -> sampled image (fragment)
    //   set 2 binding 1 -> sampler       (fragment)
    gpu::PipelineLayoutDescription graphicsLayoutDesc{};
    graphicsLayoutDesc.resourceBindings.push_back({
        0,
        {.vulkan = 0, .metal = 2},
        gpu::ResourceType::UniformBuffer,
        gpu::ShaderStage::Vertex,
        1
    });
    graphicsLayoutDesc.resourceBindings.push_back({
        1,
        {.vulkan = 0, .metal = 3},
        gpu::ResourceType::UniformBuffer,
        gpu::ShaderStage::Vertex,
        1
    });
    graphicsLayoutDesc.resourceBindings.push_back({
        2,
        {.vulkan = 0, .metal = 0},
        gpu::ResourceType::SampledImage,
        gpu::ShaderStage::Fragment,
        1
    });
    graphicsLayoutDesc.resourceBindings.push_back({
        2,
        {.vulkan = 1, .metal = 1},
        gpu::ResourceType::Sampler,
        gpu::ShaderStage::Fragment,
        1
    });
    graphicsLayoutDesc.debugName = "Cube pipeline layout";
    auto graphicsLayout = device->createPipelineLayout(graphicsLayoutDesc);
    cleanup.push(graphicsLayout);

    // 10. Procedural 2×2 texture + sampler
    const std::array<std::array<uint8_t, 4>, 4> texels = {{
        {{255,   0,   0, 255}}, {{  0, 255,   0, 255}},
        {{  0,   0, 255, 255}}, {{255, 255,   0, 255}}
    }};
    auto texture = device->createImage({
        .size = {2, 2},
        .format = gpu::PixelFormat::R8G8B8A8_UNORM,
        .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst,
        .debugName = "Procedural 2x2 texture"
    });
    texture->uploadRGBA8(texels.data(), { 2, 2 });
    device->immediateSubmit([texture](gpu::CommandBuffer* cmd)
    {
        cmd->transition(texture.get(), gpu::ImageLayout::ShaderReadOnly);
    });
    cleanup.push(texture);

    auto sampler = device->createSampler({ .debugName = "Default linear sampler" });
    cleanup.push(sampler);

    // Material resource set (set 2): texture + sampler, persistent.
    auto textureSet = device->createResourceSet(graphicsLayout.get(), 2, "Material resource set");
    textureSet->setSampledImage({.vulkan = 0, .metal = 0}, texture.get());
    textureSet->setSampler({.vulkan = 1, .metal = 1}, sampler.get());
    textureSet->update();
    cleanup.push(textureSet);

    // 11. Camera UBO (set 0): persistent for the whole application.
    const float aspect = 800.0f / 600.0f;
    auto projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
    auto view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    CameraUBO cameraData{};
    cameraData.projectionView = projection * view;

    auto cameraUbo = device->createBuffer("Camera UBO");
    cameraUbo->createUniformBuffer(cameraData);
    cleanup.push(cameraUbo);

    auto cameraSet = device->createResourceSet(graphicsLayout.get(), 0, "Camera resource set");
    cameraSet->setUniformBuffer({.vulkan = 0, .metal = 2}, cameraUbo);
    cameraSet->update();
    cleanup.push(cameraSet);

    // 12. Per-frame model UBO + resource set, duplicated through the swapchain.
    gpu::FrameResourceRing<gpu::Buffer> modelUboRing;
    modelUboRing.create(surface.get(), [&](uint32_t i)
    {
        auto buffer = device->createBuffer("Model UBO ring[" + std::to_string(i) + "]");
        buffer->createUniformBuffer(ModelUBO{});
        return buffer;
    });

    gpu::FrameResourceRing<gpu::ResourceSet> modelSetRing;
    modelSetRing.create(surface.get(), [&](uint32_t i)
    {
        auto set = device->createResourceSet(graphicsLayout.get(), 1,
            "Model resource set ring[" + std::to_string(i) + "]");
        set->setUniformBuffer({.vulkan = 0, .metal = 3}, modelUboRing.sharedAt(i));
        set->update();
        return set;
    });

    // 13. GPU mesh and cleanup manager for deferred destruction
    gpu::MeshPU gpuMesh;

    std::unique_ptr<geo::MeshPU> meshData;
    auto createGeometry = [&](bool sphere) {
        if (sphere) {
            meshData.reset(bg2e::geo::createSpherePU(0.8f, 24, 24));
        } else {
            meshData.reset(bg2e::geo::createCubePU(1.0f, 1.0f, 1.0f));
        }
        gpuMesh.setMeshData(*meshData);
        gpuMesh.build(device.get());
    };

    createGeometry(false);

    // 14. Graphics pipeline (depth enabled because depthFormat is set).
    auto colorFormat = surface->colorFormat();
    auto depthFormat = surface->depthFormat();

    gpu::GraphicsPipelineDescription pipelineDesc{};
    pipelineDesc.vertexShader   = vs.get();
    pipelineDesc.fragmentShader = fs.get();
    pipelineDesc.layout         = graphicsLayout.get();
    pipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
    pipelineDesc.colorFormat    = colorFormat;
    pipelineDesc.depthFormat    = depthFormat;
    pipelineDesc.debugName      = "Cube graphics pipeline";
    pipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
    auto pipeline = device->createGraphicsPipeline(pipelineDesc);
    cleanup.push(pipeline);

    // 15. Render loop with deferred geometry switching
    auto& graphicsQueue = device->graphicsQueue();

    bool running = true;
    bool useSphere = false;
    uint32_t lastSwitchTime = SDL_GetTicks();
    bool firstFrame = true;

    while (running)
    {
        // SDL event handling
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE) running = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                device->waitIdle();
                cleanup.flushAllDeferred();
                surface->resize({
                    static_cast<uint32_t>(event.window.data1),
                    static_cast<uint32_t>(event.window.data2)
                });
            }
        }

        const float t = static_cast<float>(SDL_GetTicks64()) / 1000.0f;
        uint32_t currentTime = SDL_GetTicks();

        // Switch geometry every 5 seconds
        if (firstFrame || currentTime - lastSwitchTime >= 5000)
        {
            if (!firstFrame)
            {
                // Schedule deferred destruction of current GPU mesh buffers.
                auto oldMesh = std::move(gpuMesh);

                cleanup.defer([oldMesh = std::move(oldMesh), useSphere]() mutable {
                    bg2e_log_debug << "Removing mesh " << (useSphere ? "sphere" : "cube") << bg2e_log_end;
                    oldMesh.cleanup();
                });

                useSphere = !useSphere;
            }

            bg2e_log_debug << "Creating new mesh " << (useSphere ? "sphere" : "cube") << bg2e_log_end;
            createGeometry(useSphere);
            lastSwitchTime = currentTime;
            firstFrame = false;
        }

        // Update model UBO (rotation)
        ModelUBO modelData{};
        modelData.model = glm::rotate(glm::mat4(1.0f), t,
            glm::normalize(glm::vec3(0.4f, 1.0f, 0.2f)));

        auto frame = surface->beginFrame();
        auto cmd = graphicsQueue.createCommandBuffer("Frame command buffer");

        auto* modelUbo = modelUboRing.current();
        modelUbo->updateUniformBuffer(modelData);
        auto* modelSet = modelSetRing.current();

        cmd->begin();

        cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
        cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
        cmd->beginRendering(frame.get());
        cmd->clearColor(0, gpu::Color(0.05f, 0.05f, 0.08f, 1.0f));
        cmd->clearDepth(1.0f);

        cmd->bindPipeline(pipeline.get());
        cmd->bindResourceSet(pipeline.get(), 0, cameraSet.get());
        cmd->bindResourceSet(pipeline.get(), 1, modelSet);
        cmd->bindResourceSet(pipeline.get(), 2, textureSet.get());
        gpuMesh.draw(cmd.get());

        cmd->endRendering();
        cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

        surface->present(cmd.get());
        cmd->end();
        graphicsQueue.submit(cmd.get());
        surface->endFrame(frame.get());

        // Flush deferred cleanups AFTER endFrame() (fence has been waited)
        cleanup.flushDeferred();
    }

    // 16. Cleanup
    device->waitIdle();
    cleanup.flushAllDeferred();

    modelSetRing.cleanup();
    modelUboRing.cleanup();
    gpuMesh.cleanup();
    cleanup.flush();

    surface->cleanup();
    device->cleanup();
    instance->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
