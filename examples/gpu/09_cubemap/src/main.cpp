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

// Example 09 — Cubemap with mirror reflection
//
// 1. Loads an equirectangular HDR image as a 2D texture.
// 2. Creates a gpu::CubeMap render target.
// 3. Renders a flipped sphere into the six cubemap faces.
// 4. Transitions the cubemap to ShaderReadOnly.
// 5. Renders a mirror cube on screen using the cubemap, with
//    per-pixel reflection computed from the surface normal and camera position.

#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <bg2e/app/SDLUtils.hpp>
#include <bg2e/math/base.hpp>
#include <array>
#include <iostream>
#include <memory>

struct CubemapCameraPushConstants {
    glm::mat4 projection;
    glm::mat4 view;
};

struct CubeReflectionPushConstants {
    glm::vec3 cameraPos;
    float padding;
};

struct ModelUBO {
    glm::mat4 model;
};

struct CameraUBO {
    glm::mat4 projection;
    glm::mat4 view;
    glm::vec4 cameraPos;
};

struct CubeRenderSettingsUBO {
    uint32_t mode; // 0 = reflection, 1 = direct cubemap debug
    uint32_t _pad0 = 0;
    uint32_t _pad1 = 0;
    uint32_t _pad2 = 0;
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
        "GPU Cubemap Example",
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

    // CleanupManager owns the ordered teardown of every DeviceResource below.
    gpu::CleanupManager cleanup(surface.get());

    // 8. Create shader modules via ShaderLib
    auto shaderBasePath = base::PlatformTools::shaderPath();
    auto shaderLib = backend->createShaderLib(shaderBasePath / "gpu_cubemap");

    // Cubemap renderer shaders (for rendering equirect texture into cubemap faces)
    auto cubemapRendererVs = shaderLib->vertex("cubemap_renderer", device.get());
    auto cubemapRendererFs = shaderLib->fragment("cubemap_renderer", device.get());
    cleanup.push(cubemapRendererVs);
    cleanup.push(cubemapRendererFs);

    // Cube reflection shaders (for rendering the mirror cube with cubemap sampling)
    auto cubeReflectionVs = shaderLib->vertex("reflect_cube", device.get());
    auto cubeReflectionFs = shaderLib->fragment("reflect_cube", device.get());
    cleanup.push(cubeReflectionVs);
    cleanup.push(cubeReflectionFs);

    // 9. Cubemap renderer pipeline layout
    //   push constants (vertex): projection + view matrices
    //   set 0 binding 0: model UBO (vertex)
    //   set 1 binding 0: equirect texture (fragment)
    //   set 1 binding 1: sampler (fragment)
    gpu::PipelineLayoutDescription cubemapRendererLayoutDesc{};
    cubemapRendererLayoutDesc.pushConstants.push_back(
        // 0: offset inside push constants.  This value only affects to Vulkan shaders
        // In Metal shaders, the push constant index are 1 for vertex shader and 0 for fragment and compute shaders
        {0, sizeof(CubemapCameraPushConstants), gpu::ShaderStage::Vertex}
    );
    cubemapRendererLayoutDesc.resourceBindings.push_back({
        0, {.vulkan = 0, .metal = 2},
        gpu::ResourceType::UniformBuffer,
        gpu::ShaderStage::Vertex, 1
    });
    cubemapRendererLayoutDesc.resourceBindings.push_back({
        1, {.vulkan = 0, .metal = 0},
        gpu::ResourceType::SampledImage,
        gpu::ShaderStage::Fragment, 1
    });
    cubemapRendererLayoutDesc.resourceBindings.push_back({
        1, {.vulkan = 1, .metal = 0},
        gpu::ResourceType::Sampler,
        gpu::ShaderStage::Fragment, 1
    });
    cubemapRendererLayoutDesc.debugName = "Cubemap renderer pipeline layout";
    auto cubemapRendererLayout = device->createPipelineLayout(cubemapRendererLayoutDesc);
    cleanup.push(cubemapRendererLayout);

    // 10. Cube reflection pipeline layout
    //   push constants (fragment): cameraPos
    //   vk = set 0 binding 0 / metal = binding 2: camera UBO (vertex)
    //   vk = set 1 binding 0 / metal = binding 3: model UBO (vertex)
    //   vk = set 2 binding 0 / metal = binding 0: cubemap texture (fragment)
    //   vk = set 2 binding 1 / metal = binding 0: sampler (fragment)
    //   vk = set 3 binding 0 / metal = binding 3: render settings UBO (fragment)
    gpu::PipelineLayoutDescription cubeReflectionLayoutDesc{};
    cubeReflectionLayoutDesc.pushConstants.push_back(
        // 0 is the offset of the push constant in Vulkan. In Metal, que push constant index are 1 for vertex shader
        // and 0 for fragment and compute shader
        {0, sizeof(CubeReflectionPushConstants), gpu::ShaderStage::Fragment}
    );
    cubeReflectionLayoutDesc.resourceBindings.push_back({
        0, {.vulkan = 0, .metal = 2},
        gpu::ResourceType::UniformBuffer,
        gpu::ShaderStage::Vertex, 1
    });
    cubeReflectionLayoutDesc.resourceBindings.push_back({
        1, {.vulkan = 0, .metal = 3},
        gpu::ResourceType::UniformBuffer,
        gpu::ShaderStage::Vertex, 1
    });
    cubeReflectionLayoutDesc.resourceBindings.push_back({
        2, {.vulkan = 0, .metal = 0},
        gpu::ResourceType::SampledImage,
        gpu::ShaderStage::Fragment, 1
    });
    cubeReflectionLayoutDesc.resourceBindings.push_back({
        2, {.vulkan = 1, .metal = 0},
        gpu::ResourceType::Sampler,
        gpu::ShaderStage::Fragment, 1
    });
    cubeReflectionLayoutDesc.resourceBindings.push_back({
        3, {.vulkan = 0, .metal = 3},
        gpu::ResourceType::UniformBuffer,
        gpu::ShaderStage::Fragment, 1
    });
    cubeReflectionLayoutDesc.debugName = "Cube reflection pipeline layout";
    auto cubeReflectionLayout = device->createPipelineLayout(cubeReflectionLayoutDesc);
    cleanup.push(cubeReflectionLayout);

    // 11. Equirectangular HDR texture
    auto assetsPath = base::PlatformTools::assetPath();
    auto imageData = std::shared_ptr<base::Image>(bg2e::db::loadImage(assetsPath / "autumn_field_4k.hdr"));
    auto equirectTexture = device->createImage({
        .size = {imageData->width(), imageData->height()},
        .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
        .usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferDst,
        .debugName = "Equirect HDR texture"
    });
    equirectTexture->uploadImage(imageData.get());

    device->immediateSubmit([equirectTexture](gpu::CommandBuffer* cmd)
    {
        cmd->transition(equirectTexture.get(), gpu::ImageLayout::ShaderReadOnly);
    });
    cleanup.push(equirectTexture);

    auto sampler = device->createSampler({ .debugName = "Default linear sampler" });
    cleanup.push(sampler);

    // Equirect texture resource set (set 1): texture + sampler, persistent.
    auto equirectTextureSet = device->createResourceSet(cubemapRendererLayout.get(), 1, "Equirect texture set");
    equirectTextureSet->setSampledImage({.vulkan = 0, .metal = 0}, equirectTexture.get());
    equirectTextureSet->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
    equirectTextureSet->update();
    cleanup.push(equirectTextureSet);

    // 12. Cubemap render target
    const uint32_t cubemapSize = 512;
    auto cubeMap = device->createCubeMap({
        .size = cubemapSize,
        .format = gpu::PixelFormat::R16G16B16A16_SFLOAT,
        .usage = gpu::ImageUsage::Sampled
               | gpu::ImageUsage::ColorAttachment
               | gpu::ImageUsage::TransferSrc
               | gpu::ImageUsage::TransferDst,
        .mipLevels = 1,
        .debugName = "Skybox cubemap"
    });
    cleanup.push(cubeMap);

    // Cubemap texture resource set (set 2): cubemap image + sampler, persistent.
    auto cubemapTextureSet = device->createResourceSet(cubeReflectionLayout.get(), 2, "Cubemap texture set");
    cubemapTextureSet->setSampledCubeMap({.vulkan = 0, .metal = 0}, cubeMap.get());
    cubemapTextureSet->setSampler({.vulkan = 1, .metal = 0}, sampler.get());
    cubemapTextureSet->update();
    cleanup.push(cubemapTextureSet);

    // 13. Camera UBO (set 0): persistent for on-screen rendering.
    const float aspect = 800.0f / 600.0f;
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 1000.0f);
    glm::vec3 cameraEye(0.0f, 0.0f, 3.0f);
    glm::mat4 view = glm::lookAt(
        cameraEye,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    CameraUBO cameraData{};
    cameraData.projection = projection;
    cameraData.view = view;
    cameraData.cameraPos = glm::vec4(cameraEye, 1.0f);

    auto cameraUbo = device->createBuffer("Camera UBO");
    cameraUbo->createUniformBuffer(cameraData);
    cleanup.push(cameraUbo);

    auto cameraSet = device->createResourceSet(cubeReflectionLayout.get(), 0, "Camera resource set");
    cameraSet->setUniformBuffer({.vulkan = 0, .metal = 2}, cameraUbo);
    cameraSet->update();
    cleanup.push(cameraSet);

    // 14. Per-frame model UBO for the cubemap renderer sphere
    gpu::FrameResourceRing<gpu::Buffer> cubemapModelUboRing;
    cubemapModelUboRing.create(surface.get(), [&](uint32_t i)
    {
        auto buffer = device->createBuffer("Cubemap model UBO ring[" + std::to_string(i) + "]");
        buffer->createUniformBuffer(ModelUBO{});
        return buffer;
    });

    gpu::FrameResourceRing<gpu::ResourceSet> cubemapModelSetRing;
    cubemapModelSetRing.create(surface.get(), [&](uint32_t i)
    {
        auto set = device->createResourceSet(
            cubemapRendererLayout.get(), 0,
            "Cubemap model set ring[" + std::to_string(i) + "]"
        );
        set->setUniformBuffer({.vulkan = 0, .metal = 2}, cubemapModelUboRing.sharedAt(i));
        set->update();
        return set;
    });

    // 15. Sphere mesh (position + texCoord0) for cubemap rendering
    std::unique_ptr<bg2e::geo::MeshPU> sphereData(bg2e::geo::createSpherePU(50.0f, 16, 16, true));
    gpu::MeshPU cubemapSphere;
    cubemapSphere.setMeshData(*sphereData);
    cubemapSphere.build(device.get());

    // 16. Cube model (position + normal + texCoord) for mirror reflection
    std::unique_ptr<bg2e::geo::MeshPNU> cubeData(bg2e::geo::createCubePNU(1.0f, 1.0f, 1.0f));
    gpu::MeshPNU cube;
    cube.setMeshData(*cubeData);
    cube.build(device.get());

    // Per-frame model UBO for the cube
    gpu::FrameResourceRing<gpu::Buffer> cubeModelUboRing;
    cubeModelUboRing.create(surface.get(), [&](uint32_t i)
    {
        auto buffer = device->createBuffer("Cube model UBO ring[" + std::to_string(i) + "]");
        buffer->createUniformBuffer(ModelUBO{});
        return buffer;
    });

    gpu::FrameResourceRing<gpu::ResourceSet> cubeModelSetRing;
    cubeModelSetRing.create(surface.get(), [&](uint32_t i)
    {
        auto set = device->createResourceSet(
            cubeReflectionLayout.get(), 1,
            "Cube model set ring[" + std::to_string(i) + "]"
        );
        set->setUniformBuffer({.vulkan = 0, .metal = 3}, cubeModelUboRing.sharedAt(i));
        set->update();
        return set;
    });

    // Cube render settings UBO ring (set 3)
    gpu::FrameResourceRing<gpu::Buffer> cubeSettingsUboRing;
    cubeSettingsUboRing.create(surface.get(), [&](uint32_t i)
    {
        auto buffer = device->createBuffer("Cube settings UBO ring[" + std::to_string(i) + "]");
        buffer->createUniformBuffer(CubeRenderSettingsUBO{});
        return buffer;
    });

    gpu::FrameResourceRing<gpu::ResourceSet> cubeSettingsSetRing;
    cubeSettingsSetRing.create(surface.get(), [&](uint32_t i)
    {
        auto set = device->createResourceSet(
            cubeReflectionLayout.get(), 3,
            "Cube settings set ring[" + std::to_string(i) + "]"
        );
        set->setUniformBuffer({.vulkan = 0, .metal = 3}, cubeSettingsUboRing.sharedAt(i));
        set->update();
        return set;
    });

    // 17. Graphics pipelines
    auto colorFormat = surface->colorFormat();
    auto depthFormat = surface->depthFormat();

    // Cubemap renderer pipeline
    gpu::GraphicsPipelineDescription cubemapRendererPipelineDesc{};
    cubemapRendererPipelineDesc.vertexShader   = cubemapRendererVs.get();
    cubemapRendererPipelineDesc.fragmentShader = cubemapRendererFs.get();
    cubemapRendererPipelineDesc.layout         = cubemapRendererLayout.get();
    cubemapRendererPipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
    cubemapRendererPipelineDesc.colorFormat    = gpu::PixelFormat::R16G16B16A16_SFLOAT;
    cubemapRendererPipelineDesc.depthFormat    = gpu::PixelFormat::Undefined;
    cubemapRendererPipelineDesc.cullMode       = gpu::CullMode::None;
    cubemapRendererPipelineDesc.debugName      = "Cubemap renderer pipeline";
    cubemapRendererPipelineDesc.addVertexBufferDescription(gpu::MeshPU::vertexBufferDescription());
    auto cubemapRendererPipeline = device->createGraphicsPipeline(cubemapRendererPipelineDesc);
    cleanup.push(cubemapRendererPipeline);

    // Cube reflection pipeline
    gpu::GraphicsPipelineDescription cubeReflectionPipelineDesc{};
    cubeReflectionPipelineDesc.vertexShader   = cubeReflectionVs.get();
    cubeReflectionPipelineDesc.fragmentShader = cubeReflectionFs.get();
    cubeReflectionPipelineDesc.layout         = cubeReflectionLayout.get();
    cubeReflectionPipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
    cubeReflectionPipelineDesc.colorFormat    = colorFormat;
    cubeReflectionPipelineDesc.depthFormat    = depthFormat;
    cubeReflectionPipelineDesc.debugName      = "Cube reflection pipeline";
    cubeReflectionPipelineDesc.addVertexBufferDescription(gpu::MeshPNU::vertexBufferDescription());
    auto cubeReflectionPipeline = device->createGraphicsPipeline(cubeReflectionPipelineDesc);
    cleanup.push(cubeReflectionPipeline);

    // 18. Cubemap face camera definitions
    const float nearPlane = 0.1f;
    const float farPlane = 1000.0f;
    auto cubemapProj = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);

    struct CubemapFaceCamera {
        glm::vec3 eye;
        glm::vec3 center;
        glm::vec3 up;
    };

    std::array<CubemapFaceCamera, 6> faceCameras = {{
        { glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f) }, // +X
        { glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f) }, // -X
        { glm::vec3(0.0f), glm::vec3( 0.0f,  -1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  -1.0f) }, // +Y
        { glm::vec3(0.0f), glm::vec3( 0.0f, 1.0f,  0.0f), glm::vec3(0.0f,  0.0f, 1.0f) }, // -Y
        { glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f) }, // +Z
        { glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f) }, // -Z
    }};

    // 19. Render loop
    auto& graphicsQueue = device->graphicsQueue();

    bool running = true;
    bool debugCubemapMode = false;
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
                surface->resize({
                    static_cast<uint32_t>(event.window.data1),
                    static_cast<uint32_t>(event.window.data2)
                });
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE)
            {
                debugCubemapMode = !debugCubemapMode;
                std::cout << "Cube render mode: "
                          << (debugCubemapMode ? "cubemap debug" : "reflection")
                          << std::endl;
            }
        }

        const float t = static_cast<float>(SDL_GetTicks64()) / 1000.0f;

        auto frame = surface->beginFrame();
        auto cmd   = graphicsQueue.createCommandBuffer("Frame command buffer");

        cmd->begin();

        // --- Cubemap pass: render sphere into 6 cubemap faces ---
        ModelUBO identityModel{};
        identityModel.model = glm::mat4(1.0f);

        auto* cubemapModelUbo = cubemapModelUboRing.current();
        cubemapModelUbo->updateUniformBuffer(identityModel);
        auto* cubemapModelSet = cubemapModelSetRing.current();

        for (uint32_t face = 0; face < 6; ++face)
        {
            auto& fc = faceCameras[face];
            glm::mat4 faceView = glm::lookAt(fc.eye, fc.center, fc.up);

            CubemapCameraPushConstants cameraPushData{};
            cameraPushData.projection = cubemapProj;
            cameraPushData.view = faceView;

            cmd->transition(cubeMap->image(), gpu::ImageLayout::ColorAttachment);
            cmd->beginRendering(cubeMap.get(), static_cast<gpu::CubemapFace>(face), 0);
            cmd->clearColor(0, gpu::Color(0.0f, 0.0f, 0.0f, 1.0f));

            cmd->bindPipeline(cubemapRendererPipeline.get());
            cmd->bindResourceSet(cubemapRendererPipeline.get(), 0, cubemapModelSet);
            cmd->bindResourceSet(cubemapRendererPipeline.get(), 1, equirectTextureSet.get());
            cmd->pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(CubemapCameraPushConstants), &cameraPushData);
            cubemapSphere.draw(cmd.get());

            cmd->endRendering();
        }

        // Transition cubemap to shader readable
        cmd->transition(cubeMap->image(), gpu::ImageLayout::ShaderReadOnly);

        // --- Main pass: render mirror cube on screen using the cubemap ---
        cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
        cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
        cmd->beginRendering(frame.get());
        cmd->clearColor(0, gpu::Color(0.05f, 0.05f, 0.08f, 1.0f));
        cmd->clearDepth(1.0f);

        // Render cube with cubemap mirror reflection
        {
            ModelUBO cubeModelData{};
            cubeModelData.model = glm::rotate(glm::mat4(1.0f), t, glm::normalize(glm::vec3(4.0f, 1.0f, 0.5f)));

            auto* cubeModelUbo = cubeModelUboRing.current();
            cubeModelUbo->updateUniformBuffer(cubeModelData);
            auto* cubeModelSet = cubeModelSetRing.current();

            CubeRenderSettingsUBO settings{};
            settings.mode = debugCubemapMode ? 1u : 0u;
            auto* settingsUbo = cubeSettingsUboRing.current();
            settingsUbo->updateUniformBuffer(settings);
            auto* settingsSet = cubeSettingsSetRing.current();

            CubeReflectionPushConstants reflectionPushData{};
            reflectionPushData.cameraPos = cameraEye;

            cmd->bindPipeline(cubeReflectionPipeline.get());
            cmd->pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(CubeReflectionPushConstants), &reflectionPushData);
            cmd->bindResourceSet(cubeReflectionPipeline.get(), 0, cameraSet.get());
            cmd->bindResourceSet(cubeReflectionPipeline.get(), 1, cubeModelSet);
            cmd->bindResourceSet(cubeReflectionPipeline.get(), 2, cubemapTextureSet.get());
            cmd->bindResourceSet(cubeReflectionPipeline.get(), 3, settingsSet);

            cube.draw(cmd.get());
        }

        cmd->endRendering();
        cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

        surface->present(cmd.get());
        cmd->end();
        graphicsQueue.submit(cmd.get());
        surface->endFrame(frame.get());
    }

    // 20. Cleanup
    device->waitIdle();

    cubemapModelSetRing.cleanup();
    cubemapModelUboRing.cleanup();
    cubemapSphere.cleanup();

    cubeSettingsSetRing.cleanup();
    cubeSettingsUboRing.cleanup();

    cubeModelUboRing.cleanup();
    cubeModelSetRing.cleanup();
    cube.cleanup();

    cleanup.flush();

    surface->cleanup();
    device->cleanup();
    instance->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
