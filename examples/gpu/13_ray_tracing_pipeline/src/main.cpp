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

#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <bg2e/geo/all.hpp>
#include <bg2e/app/SDLUtils.hpp>
#include <bg2e/math/base.hpp>

struct CameraData {
    glm::mat4 viewInverse;
    glm::mat4 projInverse;
    glm::vec4 misc; // x=frameCount, y=width, z=height
};

// Per-instance surface description consumed by the path tracer. Normals are
// computed analytically in the shader (the Cornell scene is made of planes, a
// sphere and an axis-aligned cube), so no per-vertex data needs to be bound.
//   shapeType: 0 = plane, 1 = sphere, 2 = cube
struct InstanceData {
    glm::vec4 albedo;         // rgb albedo, a = roughness
    glm::vec4 emission;       // rgb emission, a = shapeType
    glm::vec4 normalOrCenter; // plane: world normal; sphere/cube: world center; w = sphere radius
    glm::vec4 extents;        // cube: world-space half-extents
};

enum class ShapeType : int { Plane = 0, Sphere = 1, Cube = 2 };

int main(int argc, char** argv)
{
    using namespace bg2e;

    (void)argc;
    (void)argv;

    // 1. Backend selection + window + instance + surface + device
    gpu::BackendType backendType = gpu::BackendType::Vulkan;
    if (base::PlatformTools::currentPlatform() == base::Platform::macOS)
    {
        std::cout << "Select backend [1=Metal, 2=Vulkan]: ";
        int choice = 0;
        std::cin >> choice;
        backendType = (choice == 2) ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
    }

    gpu::Factory::init(backendType);
    auto* backend = gpu::Factory::backend();

    app::initSdlVideoDriver();
    SDL_Init(SDL_INIT_VIDEO);

    Uint32 windowFlags = 0;
    switch (backend->windowType())
    {
        case gpu::WindowType::Vulkan: windowFlags = SDL_WINDOW_VULKAN; break;
        case gpu::WindowType::Metal:  windowFlags = SDL_WINDOW_METAL;  break;
    }

    constexpr uint32_t kWindowWidth  = 800;
    constexpr uint32_t kWindowHeight = 600;

    SDL_Window* window = SDL_CreateWindow(
        "GPU Ray Tracing Pipeline Example",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        kWindowWidth, kWindowHeight,
        windowFlags | SDL_WINDOW_RESIZABLE
    );
    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    auto* instance = backend->sharedInstance();
    instance->enableDebugMode(true);
    instance->create(window);

    std::shared_ptr<gpu::Surface> surface = backend->createWindowSurface(instance);
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    if (!physicalDevice->properties()->rayTracingSupported())
    {
        std::cerr << "Ray tracing not supported by the selected GPU" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());
    gpu::CleanupManager cleanup(surface.get());

    // 2. Load shaders
    auto shaderBasePath = base::PlatformTools::shaderPath();
    auto shaderLib = backend->createShaderLib(shaderBasePath / "gpu_ray_tracing_pipeline");
    auto rgen = shaderLib->rayGeneration("ray_tracing_pipeline", device.get());
    auto rmiss = shaderLib->miss("ray_tracing_pipeline", device.get());
    auto rchit = shaderLib->closestHit("ray_tracing_pipeline", device.get());
    cleanup.push(rgen);
    cleanup.push(rmiss);
    cleanup.push(rchit);

    // 3. Pipeline layout
    //    set 0: output image (storage) + camera UBO + HDR accumulation image
    //    set 1: acceleration structure
    //    set 2: per-instance material/shape data SSBO
    gpu::PipelineLayoutDescription layoutDesc{};
    layoutDesc.resourceBindings.push_back({
        0, {.vulkan = 0, .metal = 0}, gpu::ResourceType::StorageImage,
        gpu::ShaderStage::RayGeneration, 1
    });
    layoutDesc.resourceBindings.push_back({
        0, {.vulkan = 1, .metal = 1}, gpu::ResourceType::UniformBuffer,
        gpu::ShaderStage::RayGeneration, 1
    });
    layoutDesc.resourceBindings.push_back({
        0, {.vulkan = 2, .metal = 1}, gpu::ResourceType::StorageImage,
        gpu::ShaderStage::RayGeneration, 1
    });
    layoutDesc.resourceBindings.push_back({
        1, {.vulkan = 0, .metal = 2}, gpu::ResourceType::AccelerationStructure,
        gpu::ShaderStage::RayGeneration, 1
    });
    layoutDesc.resourceBindings.push_back({
        2, {.vulkan = 0, .metal = 3}, gpu::ResourceType::StorageBuffer,
        gpu::ShaderStage::ClosestHit, 1
    });
    layoutDesc.debugName = "RT pipeline layout";
    auto layout = device->createPipelineLayout(layoutDesc);
    cleanup.push(layout);

    // 4. Create output storage image.
    //    Must be a 4-byte/pixel format compatible with the swapchain so the
    //    final copyImage() is a valid 1:1 texel copy. R8G8B8A8_UNORM is the
    //    portable storage format that supports shader read_write on both
    //    Vulkan and Metal (a float format like R32G32B32A32_SFLOAT is 16
    //    bytes/pixel and cannot be byte-copied into the 8-bit swapchain).
    auto outputImage = device->createImage({
        {kWindowWidth, kWindowHeight}, gpu::PixelFormat::R8G8B8A8_UNORM,
        gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc,
        gpu::ImageType::Image2D, 1, 0, "RT output image"
    });
    cleanup.push(outputImage);

    // 4b. Create HDR accumulation image. The path tracer integrates many noisy
    //     samples over time (Monte Carlo), so the running average is stored in
    //     a float image to avoid the banding/freezing an 8-bit buffer would
    //     cause. It is tonemapped into the 8-bit output image each frame.
    auto accumImage = device->createImage({
        {kWindowWidth, kWindowHeight}, gpu::PixelFormat::R32G32B32A32_SFLOAT,
        gpu::ImageUsage::Storage,
        gpu::ImageType::Image2D, 1, 0, "RT accumulation image"
    });
    cleanup.push(accumImage);

    // 5. Create camera UBO
    float aspect = static_cast<float>(kWindowWidth) / static_cast<float>(kWindowHeight);
    auto projection = glm::perspective(glm::radians(40.0f), aspect, 0.1f, 100.0f);
    auto view = glm::lookAt(
        glm::vec3(0.0f, 2.7f, 15.0f),  // camera position
        glm::vec3(0.0f, 2.7f, 0.0f),  // look-at center
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    CameraData camData{};
    camData.viewInverse = glm::inverse(view);
    camData.projInverse = glm::inverse(projection);
    camData.misc = glm::vec4(1.0f, static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight), 0.0f);

    auto cameraUbo = device->createBuffer("Camera UBO");
    cameraUbo->createUniformBuffer(camData);
    cleanup.push(cameraUbo);

    // 6. Build Cornell box geometry
    struct ScenePiece {
        std::shared_ptr<gpu::MeshPN> mesh;
        glm::mat4 model = glm::mat4(1.0f);
        InstanceData instance{};
        std::vector<std::shared_ptr<gpu::RayTracingMesh>> rtMeshes;
    };

    std::vector<std::shared_ptr<ScenePiece>> pieces;
    std::vector<InstanceData> allInstanceData;

    // shapeParam: sphere -> (unused, unused, unused, radius); cube -> (halfX, halfY, halfZ, 0)
    auto addPiece = [&](bg2e::geo::MeshPN* data, const glm::mat4& model,
                        ShapeType shapeType, float roughness,
                        const glm::vec3& albedo, const glm::vec3& emission,
                        const glm::vec4& shapeParam = glm::vec4(0.0f))
    {
        auto piece = std::make_shared<ScenePiece>();
        piece->mesh = std::make_shared<gpu::MeshPN>();
        piece->mesh->setMeshData(*data);
        piece->mesh->build(device.get());
        piece->model = model;

        InstanceData& inst = piece->instance;
        inst.albedo   = glm::vec4(albedo, roughness);
        inst.emission = glm::vec4(emission, static_cast<float>(shapeType));
        inst.extents  = glm::vec4(0.0f);

        const glm::vec3 center = glm::vec3(model[3]);
        switch (shapeType)
        {
            case ShapeType::Plane:
                // Plane local normal is +Y; store the world-space normal.
                inst.normalOrCenter =
                    glm::vec4(glm::normalize(glm::mat3(model) * glm::vec3(0.0f, 1.0f, 0.0f)), 0.0f);
                break;
            case ShapeType::Sphere:
                inst.normalOrCenter = glm::vec4(center, shapeParam.w);
                break;
            case ShapeType::Cube:
                inst.normalOrCenter = glm::vec4(center, 0.0f);
                inst.extents        = glm::vec4(glm::vec3(shapeParam), 0.0f);
                break;
        }

        for (uint32_t s = 0; s < piece->mesh->submeshCount(); ++s)
        {
            piece->rtMeshes.push_back(
                device->createRayTracingMesh(piece->mesh->rayTracingMeshDescription(s))
            );
        }

        pieces.push_back(piece);
        delete data;
    };

    constexpr float kWallRoughness = 0.5f; // matte-ish walls (rough at 50%)
    constexpr float kShiny         = 0.0f; // mirror-like objects

    // Floor (matte grey)
    addPiece(bg2e::geo::createPlanePN(5.5f, 5.5f),
             glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
             ShapeType::Plane, kWallRoughness,
             glm::vec3(0.73f, 0.73f, 0.73f), glm::vec3(0.0f));

    // Ceiling (matte grey)
    addPiece(bg2e::geo::createPlanePN(5.5f, 5.5f),
             glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.5f, 0.0f)),
                         glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
             ShapeType::Plane, kWallRoughness,
             glm::vec3(0.73f, 0.73f, 0.73f), glm::vec3(0.0f));

    // Back wall (matte grey)
    addPiece(bg2e::geo::createPlanePN(5.5f, 5.5f),
             glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.75f, -2.75f)),
                         glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
             ShapeType::Plane, kWallRoughness,
             glm::vec3(0.73f, 0.73f, 0.73f), glm::vec3(0.0f));

    // Left wall (matte red)
    addPiece(bg2e::geo::createPlanePN(5.5f, 5.5f),
             glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(-2.75f, 2.75f, 0.0f)),
                         glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
             ShapeType::Plane, kWallRoughness,
             glm::vec3(0.63f, 0.06f, 0.06f), glm::vec3(0.0f));

    // Right wall (matte green)
    addPiece(bg2e::geo::createPlanePN(5.5f, 5.5f),
             glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(2.75f, 2.75f, 0.0f)),
                         glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
             ShapeType::Plane, kWallRoughness,
             glm::vec3(0.12f, 0.45f, 0.15f), glm::vec3(0.0f));

    // Light rectangle at ceiling (emissive, black albedo so paths terminate on it)
    addPiece(bg2e::geo::createPlanePN(1.3f, 1.3f),
             glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.49f, 0.0f)),
             ShapeType::Plane, kWallRoughness,
             glm::vec3(0.0f), glm::vec3(18.0f, 18.0f, 18.0f));

    // Cube (shiny / mirror-like)
    addPiece(bg2e::geo::createCubePN(1.2f, 1.2f, 1.2f),
             glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.6f, 0.5f)),
             ShapeType::Cube, kShiny,
             glm::vec3(0.95f, 0.95f, 0.95f), glm::vec3(0.0f),
             glm::vec4(0.6f, 0.6f, 0.6f, 0.0f));

    // Sphere (shiny / mirror-like)
    addPiece(bg2e::geo::createSpherePN(0.6f, 32, 24),
             glm::translate(glm::mat4(1.0f), glm::vec3(1.2f, 0.6f, -0.5f)),
             ShapeType::Sphere, kShiny,
             glm::vec3(0.95f, 0.95f, 0.95f), glm::vec3(0.0f),
             glm::vec4(0.0f, 0.0f, 0.0f, 0.6f));

    // 7. Build BLAS
    device->immediateSubmit([&](gpu::CommandBuffer* cmd)
    {
        for (auto& piece : pieces)
        {
            for (auto& rtMesh : piece->rtMeshes)
            {
                cmd->buildRayTracingMesh(rtMesh.get());
            }
        }
    });

    // 8. Create TLAS
    auto rayTracingScene = device->createRayTracingScene("Cornell box scene");
    cleanup.push(rayTracingScene);

    // The instance metadata array is built in lock-step with addInstance so
    // that gl_InstanceCustomIndexEXT (Vulkan) / intersection.instance_id
    // (Metal) directly indexes the matching InstanceData entry.
    uint32_t instanceId = 0;
    for (auto& piece : pieces)
    {
        for (auto& rtMesh : piece->rtMeshes)
        {
            rayTracingScene->addInstance(rtMesh.get(), piece->model, instanceId++, 0xFF);
            allInstanceData.push_back(piece->instance);
        }
    }

    device->immediateSubmit([&](gpu::CommandBuffer* cmd)
    {
        rayTracingScene->buildOrUpdate(cmd);
    });

    // 9. Create instance data SSBO
    auto instanceBuffer = device->createBuffer("Instance Data SSBO");
    instanceBuffer->createStorageBuffer(allInstanceData.data(),
        allInstanceData.size() * sizeof(InstanceData));
    cleanup.push(instanceBuffer);

    // 10. Create resource sets
    auto outputSet = device->createResourceSet(layout.get(), 0, "Output set");
    outputSet->setStorageImage({.vulkan = 0, .metal = 0}, outputImage.get());
    outputSet->setUniformBuffer({.vulkan = 1, .metal = 1}, cameraUbo.get());
    outputSet->setStorageImage({.vulkan = 2, .metal = 1}, accumImage.get());
    outputSet->update();
    cleanup.push(outputSet);

    auto sceneSet = device->createResourceSet(layout.get(), 1, "Scene set");
    sceneSet->setRayTracingScene({.vulkan = 0, .metal = 2}, rayTracingScene.get());
    sceneSet->update();
    cleanup.push(sceneSet);

    auto dataSet = device->createResourceSet(layout.get(), 2, "Data set");
    dataSet->setStorageBuffer({.vulkan = 0, .metal = 3}, instanceBuffer.get());
    dataSet->update();
    cleanup.push(dataSet);

    // 11. Create RayTracingPipeline
    gpu::RayTracingPipelineDescription pipelineDesc{};
    pipelineDesc.raygenShader = rgen.get();
    pipelineDesc.missShader = rmiss.get();
    pipelineDesc.closestHitShader = rchit.get();
    pipelineDesc.layout = layout.get();
    pipelineDesc.maxRecursionDepth = 1;
    pipelineDesc.debugName = "Cornell box RT pipeline";
    auto rtPipeline = device->createRayTracingPipeline(pipelineDesc);
    cleanup.push(rtPipeline);

    // 12. Render loop
    auto& graphicsQueue = device->graphicsQueue();
    uint32_t frameCount = 0;
    bool running = true;

    // Progressive accumulation state. The ray generation shader blends each
    // new frame into the output with weight 1/frameCount, so frameCount must
    // be reset to 0 whenever the accumulated result becomes invalid (camera
    // moved, or the output image was recreated). Otherwise the old frames
    // dominate the running average and changes to the camera produce no
    // visible effect.
    glm::mat4 lastView = camData.viewInverse;
    glm::mat4 lastProj = camData.projInverse;

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
                uint32_t newWidth  = static_cast<uint32_t>(event.window.data1);
                uint32_t newHeight = static_cast<uint32_t>(event.window.data2);
                device->waitIdle();
                surface->resize({newWidth, newHeight});

                // Recreate output and accumulation images for the new size
                outputImage->cleanup();
                auto newOutput = device->createImage({
                    {newWidth, newHeight}, gpu::PixelFormat::R8G8B8A8_UNORM,
                    gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc,
                    gpu::ImageType::Image2D, 1, 0, "RT output image"
                });
                cleanup.push(newOutput);

                accumImage->cleanup();
                auto newAccum = device->createImage({
                    {newWidth, newHeight}, gpu::PixelFormat::R32G32B32A32_SFLOAT,
                    gpu::ImageUsage::Storage,
                    gpu::ImageType::Image2D, 1, 0, "RT accumulation image"
                });
                cleanup.push(newAccum);

                // Update resource set with the new images
                outputSet->cleanup();
                auto newOutputSet = device->createResourceSet(layout.get(), 0, "Output set");
                newOutputSet->setStorageImage({.vulkan = 0, .metal = 0}, newOutput.get());
                newOutputSet->setUniformBuffer({.vulkan = 1, .metal = 1}, cameraUbo.get());
                newOutputSet->setStorageImage({.vulkan = 2, .metal = 1}, newAccum.get());
                newOutputSet->update();
                cleanup.push(newOutputSet);

                outputImage = std::move(newOutput);
                accumImage  = std::move(newAccum);
                outputSet = std::move(newOutputSet);

                camData.misc.y = static_cast<float>(newWidth);
                camData.misc.z = static_cast<float>(newHeight);
                cameraUbo->updateUniformBuffer(camData);

                // The new output image is uninitialized: restart accumulation
                // so the next frame writes its colour directly instead of
                // blending against garbage.
                frameCount = 0;
            }
        }

        // Restart accumulation whenever the camera changes, so a moved camera
        // is reflected immediately instead of being averaged away.
        if (camData.viewInverse != lastView || camData.projInverse != lastProj)
        {
            frameCount = 0;
            lastView = camData.viewInverse;
            lastProj = camData.projInverse;
        }

        frameCount++;
        camData.misc.x = static_cast<float>(frameCount);
        cameraUbo->updateUniformBuffer(camData);

        auto frame = surface->beginFrame();
        if (!frame) continue;

        auto cmd = graphicsQueue.createCommandBuffer("Frame");

        cmd->begin();

        // Transition storage images to general layout for ray tracing
        cmd->transition(outputImage.get(), gpu::ImageLayout::General);
        cmd->transition(accumImage.get(), gpu::ImageLayout::General);

        // Bind RT pipeline and trace rays
        cmd->bindPipeline(rtPipeline.get());
        cmd->bindResourceSet(rtPipeline.get(), 0, outputSet.get());
        cmd->bindResourceSet(rtPipeline.get(), 1, sceneSet.get());
        cmd->bindResourceSet(rtPipeline.get(), 2, dataSet.get());
        cmd->traceRays(outputImage->width(), outputImage->height(), 1);

        // Copy output to swapchain
        cmd->transition(outputImage.get(), gpu::ImageLayout::TransferSrc);
        cmd->transition(frame->colorImage(), gpu::ImageLayout::TransferDst);
        cmd->copyImage(outputImage.get(), frame->colorImage());
        cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

        surface->present(cmd.get());

        cmd->end();
        graphicsQueue.submit(cmd.get());
        surface->endFrame(frame.get());
    }

    // 13. Cleanup
    device->waitIdle();
    for (auto& piece : pieces)
    {
        for (auto& rtMesh : piece->rtMeshes) { rtMesh->cleanup(); }
        piece->mesh->cleanup();
    }
    pieces.clear();
    cleanup.flush();
    surface->cleanup();
    device->cleanup();
    instance->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
