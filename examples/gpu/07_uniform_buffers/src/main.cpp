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

// Example 07 — Uniform buffers
//
// Renders a rotating, textured cube. It demonstrates the gpu:: uniform-buffer
// support added on top of example 05:
//   - A persistent camera UBO + resource set (lives for the whole application).
//   - A per-frame model UBO duplicated with FrameResourceRing<gpu::Buffer> and a
//     matching FrameResourceRing<gpu::ResourceSet>.
//   - Separate descriptor sets: set 0 = camera, set 1 = model, set 2 = material.
//   - Depth testing for correct cube occlusion.
//   - gpu::CleanupManager for ordered teardown of DeviceResource objects.

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
        "GPU Uniform Buffers Example",
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

    // CleanupManager owns the ordered teardown of every DeviceResource below.
    gpu::CleanupManager cleanup;

    // 8. Create shader modules
    auto shaderBasePath = base::PlatformTools::shaderPath();
    std::string targetName = "gpu_uniform_buffers";

    std::shared_ptr<gpu::ShaderModule> vs;
    std::shared_ptr<gpu::ShaderModule> fs;

    if (backendType == gpu::BackendType::Vulkan)
    {
        auto vsPath = (shaderBasePath / targetName / "cube.vert.spv").string();
        auto fsPath = (shaderBasePath / targetName / "cube.frag.spv").string();
        vs = device->createShaderModule({ vsPath, "main", gpu::ShaderStage::Vertex,   "Cube vertex shader" });
        fs = device->createShaderModule({ fsPath, "main", gpu::ShaderStage::Fragment, "Cube fragment shader" });
    }
    else
    {
        auto libPath = (shaderBasePath / targetName / "metal" / "cube.metallib").string();
        vs = device->createShaderModule({ libPath, "cube_vertex",   gpu::ShaderStage::Vertex,   "Cube vertex shader" });
        fs = device->createShaderModule({ libPath, "cube_fragment", gpu::ShaderStage::Fragment, "Cube fragment shader" });
    }
    cleanup.push(vs);
    cleanup.push(fs);

    // 9. Pipeline layout with separate sets:
    //   set 0 binding 0 -> camera UBO  (vertex)
    //   set 1 binding 0 -> model UBO   (vertex)
    //   set 2 binding 0 -> sampled image (fragment)
    //   set 2 binding 1 -> sampler       (fragment)
    gpu::PipelineLayoutDescription graphicsLayoutDesc{};
    graphicsLayoutDesc.resourceBindings.push_back({ 0, 0, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex,   1 });
    graphicsLayoutDesc.resourceBindings.push_back({ 1, 0, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex,   1 });
    graphicsLayoutDesc.resourceBindings.push_back({ 2, 0, gpu::ResourceType::SampledImage,  gpu::ShaderStage::Fragment, 1 });
    graphicsLayoutDesc.resourceBindings.push_back({ 2, 1, gpu::ResourceType::Sampler,       gpu::ShaderStage::Fragment, 1 });
    graphicsLayoutDesc.debugName = "Cube pipeline layout";
    auto graphicsLayout = device->createPipelineLayout(graphicsLayoutDesc);
    cleanup.push(graphicsLayout);

    // 10. Procedural 2×2 texture + sampler
    const std::array<std::array<uint8_t, 4>, 4> texels = {{
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
    cleanup.push(texture);

    auto sampler = device->createSampler({ .debugName = "Default linear sampler" });
    cleanup.push(sampler);

    // Material resource set (set 2): texture + sampler, persistent.
    auto textureSet = device->createResourceSet(graphicsLayout.get(), 2, "Material resource set");
    textureSet->setSampledImage(0, texture.get());
    textureSet->setSampler(1, sampler.get());
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
    cameraSet->setUniformBuffer(0, cameraUbo);
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
        set->setUniformBuffer(0, modelUboRing.sharedAt(i));
        set->update();
        return set;
    });

    // 13. Cube mesh (position + texCoord0) built from the procedural geometry API.
    std::unique_ptr<bg2e::geo::MeshPU> cubeData(bg2e::geo::createCubePU(1.0f, 1.0f, 1.0f));

    gpu::MeshPU cube;
    cube.setMeshData(*cubeData);
    cube.build(device.get());

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

    // 15. Render loop
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

        // Rotate the cube around a tilted axis.
        ModelUBO modelData{};
        modelData.model = glm::rotate(glm::mat4(1.0f), t, glm::normalize(glm::vec3(0.4f, 1.0f, 0.2f)));

        auto frame = surface->beginFrame();
        auto cmd   = graphicsQueue.createCommandBuffer("Frame command buffer");

        // Update + bind the per-frame model resources.
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
        cube.draw(cmd.get());

        cmd->endRendering();
        cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

        surface->present(cmd.get());
        cmd->end();
        graphicsQueue.submit(cmd.get());
        surface->endFrame(frame.get());
    }

    // 16. Cleanup
    device->waitIdle();

    modelSetRing.cleanup();
    modelUboRing.cleanup();
    cube.cleanup();           // gpu::MeshPU is not a DeviceResource
    cleanup.flush();          // DeviceResource objects, in reverse push order

    surface->cleanup();
    device->cleanup();
    instance->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
