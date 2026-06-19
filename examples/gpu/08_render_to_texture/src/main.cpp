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

// Example 08 — Render to texture
//
// Renders a rotating, textured cube into an offscreen color image, runs a
// compute Sobel edge-detection pass into a second image, then copies the
// processed result into the current surface image for presentation.
//
// Demonstrates:
//   - Rendering into a regular 2D gpu::Image (offscreen color + depth).
//   - Compute shader reading from one storage image and writing to another.
//   - Image copy between gpu::Image objects.
//   - Explicit layout transitions through the full render → compute → present pipeline.

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

// ---------------------------------------------------------------------------
// Helper: create the three offscreen images used by the example.
// ---------------------------------------------------------------------------
static void createOffscreenImages(
    bg2e::gpu::Device* device,
    const bg2e::gpu::Size2D& size,
    std::shared_ptr<bg2e::gpu::Image>& offscreenColor,
    std::shared_ptr<bg2e::gpu::Image>& offscreenDepth,
    std::shared_ptr<bg2e::gpu::Image>& computeOutput,
    const std::string& tag)
{
    using namespace bg2e;
    // Offscreen color: render target + sampled + transfer source
    offscreenColor = device->createImage({
        .size = size,
        .format = gpu::PixelFormat::R8G8B8A8_UNORM,
        .usage = gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::Storage,
        .debugName = "Offscreen color " + tag
    });

    // Offscreen depth: depth/stencil + sampled
    offscreenDepth = device->createImage({
        .size = size,
        .format = gpu::PixelFormat::D32_SFLOAT,
        .usage = gpu::ImageUsage::DepthStencil | gpu::ImageUsage::Sampled,
        .debugName = "Offscreen depth " + tag
    });

    // Compute output: storage + transfer source + transfer destination
    computeOutput = device->createImage({
        .size = size,
        .format = gpu::PixelFormat::R8G8B8A8_UNORM,
        .usage = gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst | gpu::ImageUsage::Storage,
        .debugName = "Compute output " + tag
    });
}

// ---------------------------------------------------------------------------
// Helper: recreate compute resource set after images change.
// ---------------------------------------------------------------------------
static std::shared_ptr<bg2e::gpu::ResourceSet> createComputeResourceSet(
    bg2e::gpu::Device* device,
    bg2e::gpu::PipelineLayout* layout,
    bg2e::gpu::Image* inputImage,
    bg2e::gpu::Image* outputImage)
{
    auto set = device->createResourceSet(layout, 0, "Compute resource set");
    set->setStorageImage({.vulkan = 0, .metal = 0}, inputImage);
    set->setStorageImage({.vulkan = 1, .metal = 1}, outputImage);
    set->update();
    return set;
}

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

    // 3. SDL init + create window
    app::initSdlVideoDriver();
    SDL_Init(SDL_INIT_VIDEO);

    Uint32 windowFlags = 0;
    switch (backend->windowType())
    {
        case gpu::WindowType::Vulkan: windowFlags = SDL_WINDOW_VULKAN; break;
        case gpu::WindowType::Metal:  windowFlags = SDL_WINDOW_METAL;  break;
    }

    SDL_Window* window = SDL_CreateWindow(
        "GPU Render to Texture Example",
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

    // 5. Create surface
    std::shared_ptr<gpu::Surface> surface = backend->createWindowSurface(instance);

    // 6. Select physical device
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    // 7. Create logical device
    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());

    // CleanupManager owns the ordered teardown of every DeviceResource below.
    gpu::CleanupManager cleanup(surface.get());

    // 8. Create shader modules via ShaderLib
    auto shaderBasePath = base::PlatformTools::shaderPath();

    auto shaderLib = backend->createShaderLib(shaderBasePath / "gpu_render_to_texture");
    auto vs = shaderLib->vertex("cube", device.get());
    auto fs = shaderLib->fragment("cube", device.get());
    auto cs = shaderLib->compute("edge_filter", device.get());
    cleanup.push(vs);
    cleanup.push(fs);
    cleanup.push(cs);

    // 9. Graphics pipeline layout (same as 07):
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

    // 10. Compute pipeline layout:
    //   set 0 binding 0 -> storage image (input,  read-only in practice)
    //   set 0 binding 1 -> storage image (output, writable)
    gpu::PipelineLayoutDescription computeLayoutDesc{};
    computeLayoutDesc.resourceBindings.push_back({ 0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::StorageImage, gpu::ShaderStage::Compute, 1 });
    computeLayoutDesc.resourceBindings.push_back({ 0, {.vulkan = 1, .metal = 1}, gpu::ResourceType::StorageImage, gpu::ShaderStage::Compute, 1 });
    computeLayoutDesc.debugName = "Edge filter pipeline layout";
    auto computeLayout = device->createPipelineLayout(computeLayoutDesc);
    cleanup.push(computeLayout);

    // 11. Procedural 2x2 texture + sampler (same as 07)
    const std::array<std::array<uint8_t, 4>, 4> texels = {{
        {{255,   0,   0, 255}}, {{  0, 255,   0, 255}},
        {{  0,   0, 255, 255}}, {{255, 255,   0, 255}}
    }};
    auto texture = device->createImage({
        .size = {2, 2},
        .format = gpu::PixelFormat::R8G8B8A8_UNORM,
        .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst,
        .debugName ="Procedural 2x2 texture"
    });

    texture->uploadRGBA8(texels.data(), { 2, 2 });
    device->immediateSubmit([texture](gpu::CommandBuffer* cmd)
    {
        cmd->transition(texture.get(), gpu::ImageLayout::ShaderReadOnly);
    });
    cleanup.push(texture);

    auto sampler = device->createSampler({ .debugName = "Default linear sampler" });
    cleanup.push(sampler);

    // Material resource set (set 2)
    auto textureSet = device->createResourceSet(graphicsLayout.get(), 2, "Material resource set");
    textureSet->setSampledImage({.vulkan = 0, .metal = 0}, texture.get());
    textureSet->setSampler({.vulkan = 1, .metal = 1}, sampler.get());
    textureSet->update();
    cleanup.push(textureSet);

    // 12. Camera UBO (set 0): persistent for the whole application.
    float aspect = 800.0f / 600.0f;
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
    cameraSet->setUniformBuffer({.vulkan = 0, .metal = 2 }, cameraUbo);
    cameraSet->update();
    cleanup.push(cameraSet);

    // 13. Per-frame model UBO ring (same as 07)
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
            "Model resource set ring[" + std::to_string(i) + "]"
        );
        set->setUniformBuffer({.vulkan = 0, .metal = 3}, modelUboRing.sharedAt(i));
        set->update();
        return set;
    });

    // 14. Cube mesh (same as 07)
    std::unique_ptr<bg2e::geo::MeshPU> cubeData(bg2e::geo::createCubePU(1.0f, 1.0f, 1.0f));

    gpu::MeshPU cube;
    cube.setMeshData(*cubeData);
    cube.build(device.get());

    // 15. Graphics pipeline — render into offscreen color format
    auto offscreenColorFormat = gpu::PixelFormat::R8G8B8A8_UNORM;
    auto offscreenDepthFormat = gpu::PixelFormat::D32_SFLOAT;

    gpu::GraphicsPipelineDescription pipelineDesc{};
    pipelineDesc.vertexShader   = vs.get();
    pipelineDesc.fragmentShader = fs.get();
    pipelineDesc.layout         = graphicsLayout.get();
    pipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
    pipelineDesc.colorFormat    = offscreenColorFormat;
    pipelineDesc.depthFormat    = offscreenDepthFormat;
    pipelineDesc.debugName      = "Cube graphics pipeline";
    pipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
    auto pipeline = device->createGraphicsPipeline(pipelineDesc);
    cleanup.push(pipeline);

    // 16. Compute pipeline (edge filter)
    gpu::ComputePipelineDescription computePipelineDesc{};
    computePipelineDesc.computeShader = cs.get();
    computePipelineDesc.layout        = computeLayout.get();
    computePipelineDesc.debugName     = "Edge filter compute pipeline";
    auto computePipeline = device->createComputePipeline(computePipelineDesc);
    cleanup.push(computePipeline);

    // 17. Create offscreen images matching the surface size
    // NOTE: these are NOT pushed to CleanupManager because they are recreated on resize.
    // They are cleaned up manually in the cleanup section below.
    auto surfaceSize = surface->size();
    std::shared_ptr<gpu::Image> offscreenColor;
    std::shared_ptr<gpu::Image> offscreenDepth;
    std::shared_ptr<gpu::Image> computeOutput;
    createOffscreenImages(device.get(), surfaceSize, offscreenColor, offscreenDepth, computeOutput, "init");

    // 18. Compute resource set (input: offscreenColor, output: computeOutput)
    // Also not pushed to CleanupManager — recreated on resize.
    auto computeSet = createComputeResourceSet(
        device.get(), computeLayout.get(),
        offscreenColor.get(), computeOutput.get()
    );

    // 19. Render loop
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
                device->waitIdle();
                uint32_t w = event.window.data1;
                uint32_t h = event.window.data2;
                if (w == 0 || h == 0) continue;
                surface->resize({w, h});

                // Recreate offscreen images to match new surface size
                offscreenColor->cleanup();
                offscreenColor = device->createImage({
                    .size ={w, h},
                    .format = gpu::PixelFormat::R8G8B8A8_UNORM,
                    .usage = gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::Storage,
                    .debugName = "Offscreen color"
                });

                offscreenDepth->cleanup();
                offscreenDepth = device->createImage({
                    .size = {w, h},
                    .format = gpu::PixelFormat::D32_SFLOAT,
                    .usage = gpu::ImageUsage::DepthStencil | gpu::ImageUsage::Sampled,
                    .debugName = "Offscreen depth"
                });

                computeOutput->cleanup();
                computeOutput = device->createImage({
                    .size = {w, h},
                    .format = gpu::PixelFormat::R8G8B8A8_UNORM,
                    .usage = gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst | gpu::ImageUsage::Storage,
                    .debugName = "Compute output"
                });

                // Recreate compute resource set with new images
                computeSet->cleanup();
                computeSet = createComputeResourceSet(
                    device.get(), computeLayout.get(),
                    offscreenColor.get(), computeOutput.get()
                );

                // Recreate the projection matrix with the new aspect ratio
                aspect = static_cast<float>(w) / static_cast<float>(h);
                projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
                view = glm::lookAt(
                    glm::vec3(0.0f, 0.0f, 3.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );
                cameraData.projectionView = projection * view;
                cameraUbo->updateUniformBuffer(cameraData);
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

        // --- Pass 1: Render cube into offscreen color/depth ---
        cmd->transition(offscreenColor.get(), gpu::ImageLayout::ColorAttachment);
        cmd->transition(offscreenDepth.get(), gpu::ImageLayout::DepthAttachment);
        cmd->beginRendering(offscreenColor.get(), offscreenDepth.get());
        cmd->clearColor(0, gpu::Color(0.05f, 0.05f, 0.08f, 1.0f));
        cmd->clearDepth(1.0f);

        cmd->bindPipeline(pipeline.get());
        cmd->bindResourceSet(pipeline.get(), 0, cameraSet.get());
        cmd->bindResourceSet(pipeline.get(), 1, modelSet);
        cmd->bindResourceSet(pipeline.get(), 2, textureSet.get());
        cube.draw(cmd.get());

        cmd->endRendering();

        // --- Pass 2: Compute edge detection ---
        cmd->transition(offscreenColor.get(), gpu::ImageLayout::General);
        cmd->transition(computeOutput.get(), gpu::ImageLayout::General);
        cmd->beginCompute();
        cmd->bindPipeline(computePipeline.get());
        cmd->bindResourceSet(computePipeline.get(), 0, computeSet.get());
        uint32_t w = surface->size().width;
        uint32_t h = surface->size().height;
        cmd->dispatch((w + 15) / 16, (h + 15) / 16, 1);
        cmd->endCompute();

        // --- Pass 3: Copy postprocessed image to surface ---
        cmd->transition(computeOutput.get(), gpu::ImageLayout::TransferSrc);
        cmd->transition(frame->colorImage(), gpu::ImageLayout::TransferDst);
        cmd->copyImage(computeOutput.get(), frame->colorImage());
        cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

        surface->present(cmd.get());
        cmd->end();
        graphicsQueue.submit(cmd.get());
        surface->endFrame(frame.get());
    }

    // 20. Cleanup
    device->waitIdle();

    modelSetRing.cleanup();
    modelUboRing.cleanup();
    cube.cleanup();

    // Clean up offscreen images and compute set manually (not in CleanupManager
    // because they are recreated on resize).
    computeSet->cleanup();
    computeOutput->cleanup();
    offscreenDepth->cleanup();
    offscreenColor->cleanup();

    cleanup.flush();

    surface->cleanup();
    device->cleanup();
    instance->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
