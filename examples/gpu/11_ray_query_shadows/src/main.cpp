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

// Example 11 — Ray query shadows
//
// Validates the first ray tracing support block of bg2e::gpu: RayTracingMesh
// (bottom-level acceleration data per submesh) and RayTracingScene (top-level
// acceleration data with instances). It renders a very simple Lambert-lit scene
// (ground plane + cube + sphere) with a single orbiting point light, and casts
// a visibility ray from each shaded fragment toward the light using hardware
// ray queries (Vulkan) / ray tracing intersectors (Metal) to produce hard
// shadows.
//
// There is no ray tracing pipeline and no SBT: normal rasterization is used for
// rendering and the acceleration structure is queried from the fragment shader.

#include <bg2e.hpp>
#include <bg2e/gpu/all.hpp>
#include <bg2e/geo/all.hpp>
#include <bg2e/app/SDLUtils.hpp>
#include <bg2e/math/base.hpp>

#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

struct CameraUBO {
    glm::mat4 projectionView;
};

struct ObjectUBO {
    glm::mat4 model;
    glm::vec4 albedo;
};

struct LightUBO {
    glm::vec4 lightPosition;   // xyz = world position
    glm::vec4 lightColor;      // rgb = color, a = intensity
    glm::vec4 ambient;         // rgb = ambient term
};

// A single renderable object: its rasterization mesh, per-object uniforms and
// the RayTracingMesh objects (one per submesh) used to populate the scene.
struct SceneObject {
    std::shared_ptr<bg2e::gpu::MeshPN> mesh;
    glm::mat4 model = glm::mat4(1.0f);
    glm::vec4 albedo = glm::vec4(1.0f);

    std::shared_ptr<bg2e::gpu::Buffer>      objectUbo;
    std::shared_ptr<bg2e::gpu::ResourceSet> objectSet;

    std::vector<std::shared_ptr<bg2e::gpu::RayTracingMesh>> rtMeshes;
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

    SDL_Window* window = SDL_CreateWindow(
        "GPU Ray Query Shadows Example",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        windowFlags | SDL_WINDOW_RESIZABLE
    );
    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return 1;
    }

    auto* instance = backend->sharedInstance();
    instance->enableDebugMode(true);
    instance->create(window);

    std::shared_ptr<gpu::Surface> surface = backend->createWindowSurface(instance);

    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    // 2. Detect hardware ray tracing / ray query support.
    if (!physicalDevice->properties()->rayTracingSupported())
    {
        bg2e_log_error << "This example requires hardware accelerated ray queries, "
                  << "which are not supported by the selected device:\n  "
                  << physicalDevice->properties()->name << bg2e_log_end;

        if (bg2e::base::PlatformTools::currentPlatform() == base::Platform::macOS &&
            backendType == gpu::BackendType::Vulkan)
        {
            bg2e_log_error << "Note that Vulkan in macOS is not compatible with ray tracing. "
                  << "Please, try using Metal instead of Vulkan" << bg2e_log_end;
        }
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());

    gpu::CleanupManager cleanup;

    // 3. Shaders.
    auto shaderBasePath = base::PlatformTools::shaderPath();
    auto shaderLib = backend->createShaderLib(shaderBasePath / "gpu_ray_query_shadows");
    auto vs = shaderLib->vertex("shadow", device.get());
    auto fs = shaderLib->fragment("shadow", device.get());
    cleanup.push(vs);
    cleanup.push(fs);

    // 4. Pipeline layout:
    //   set 0 binding 0 -> camera UBO            (vertex)
    //   set 1 binding 0 -> per-object UBO        (vertex)
    //   set 2 binding 0 -> light UBO             (fragment)
    //   set 2 binding 1 -> acceleration structure (fragment)
    gpu::PipelineLayoutDescription layoutDesc{};
    layoutDesc.resourceBindings.push_back({
        0, {.vulkan = 0, .metal = 2}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1
    });
    layoutDesc.resourceBindings.push_back({
        1, {.vulkan = 0, .metal = 3}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Vertex, 1
    });
    layoutDesc.resourceBindings.push_back({
        2, {.vulkan = 0, .metal = 1}, gpu::ResourceType::UniformBuffer, gpu::ShaderStage::Fragment, 1
    });
    layoutDesc.resourceBindings.push_back({
        2, {.vulkan = 1, .metal = 2}, gpu::ResourceType::AccelerationStructure, gpu::ShaderStage::Fragment, 1
    });
    layoutDesc.debugName = "Ray query shadows pipeline layout";
    auto graphicsLayout = device->createPipelineLayout(layoutDesc);
    cleanup.push(graphicsLayout);

    // 5. Camera UBO (set 0): persistent.
    const float aspect = 800.0f / 600.0f;
    auto projection = glm::perspective(glm::radians(55.0f), aspect, 0.1f, 100.0f);
    auto view = glm::lookAt(
        glm::vec3(4.5f, 4.0f, 5.5f),
        glm::vec3(0.0f, 0.6f, 0.0f),
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

    // 6. Build the scene objects (ground plane, cube, sphere). Each uses simple
    //    position+normal geometry with a single albedo color.
    std::vector<SceneObject> objects;

    auto addObject = [&](bg2e::geo::MeshPN* data, const glm::mat4& model, const glm::vec4& albedo)
    {
        SceneObject obj;
        obj.mesh = std::make_shared<gpu::MeshPN>();
        obj.mesh->setMeshData(*data);
        obj.mesh->build(device.get());
        obj.model = model;
        obj.albedo = albedo;

        ObjectUBO objectData{};
        objectData.model = model;
        objectData.albedo = albedo;
        obj.objectUbo = device->createBuffer("Object UBO");
        obj.objectUbo->createUniformBuffer(objectData);

        obj.objectSet = device->createResourceSet(graphicsLayout.get(), 1, "Object resource set");
        obj.objectSet->setUniformBuffer({.vulkan = 0, .metal = 3}, obj.objectUbo);
        obj.objectSet->update();

        // One RayTracingMesh per submesh, reusing the mesh GPU buffers.
        for (uint32_t s = 0; s < obj.mesh->submeshCount(); ++s)
        {
            obj.rtMeshes.push_back(
                device->createRayTracingMesh(obj.mesh->rayTracingMeshDescription(s))
            );
        }

        objects.push_back(std::move(obj));
        delete data;
    };

    addObject(bg2e::geo::createPlanePN(12.0f, 12.0f),
              glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
              glm::vec4(0.75f, 0.75f, 0.78f, 1.0f));

    addObject(bg2e::geo::createCubePN(1.2f, 1.2f, 1.2f),
              glm::translate(glm::mat4(1.0f), glm::vec3(-1.3f, 0.6f, 0.4f)),
              glm::vec4(0.85f, 0.25f, 0.20f, 1.0f));

    addObject(bg2e::geo::createSpherePN(0.75f, 32, 24),
              glm::translate(glm::mat4(1.0f), glm::vec3(1.3f, 0.75f, -0.3f)),
              glm::vec4(0.20f, 0.45f, 0.85f, 1.0f));

    // 7. Build the bottom-level acceleration structures (one GPU submit).
    device->immediateSubmit([&](gpu::CommandBuffer* cmd)
    {
        for (auto& obj : objects)
        {
            for (auto& rtMesh : obj.rtMeshes)
            {
                cmd->buildRayTracingMesh(rtMesh.get());
            }
        }
    });

    // 8. Ray tracing scene (top-level acceleration structure). The instance list
    //    is rebuilt every frame to exercise the dynamic build path, even though
    //    the geometry here is static (only the light moves).
    auto rayTracingScene = device->createRayTracingScene("Shadow ray tracing scene");
    cleanup.push(rayTracingScene);

    uint32_t instanceId = 0;
    auto populateInstances = [&]()
    {
        rayTracingScene->clearInstances();
        instanceId = 0;
        for (auto& obj : objects)
        {
            for (auto& rtMesh : obj.rtMeshes)
            {
                rayTracingScene->addInstance(rtMesh.get(), obj.model, instanceId++, 0xFF);
            }
        }
    };

    // Initial build so the acceleration structure handle exists before it is
    // bound into the light resource set.
    populateInstances();
    device->immediateSubmit([&](gpu::CommandBuffer* cmd)
    {
        rayTracingScene->buildOrUpdate(cmd);
    });

    // 9. Light UBO + acceleration structure resource set (set 2).
    auto lightUbo = device->createBuffer("Light UBO");
    lightUbo->createUniformBuffer(LightUBO{});
    cleanup.push(lightUbo);

    auto lightSet = device->createResourceSet(graphicsLayout.get(), 2, "Light resource set");
    lightSet->setUniformBuffer({.vulkan = 0, .metal = 1}, lightUbo);
    lightSet->setRayTracingScene({.vulkan = 1, .metal = 2}, rayTracingScene.get());
    lightSet->update();
    cleanup.push(lightSet);

    // 10. Graphics pipeline (depth enabled, culling disabled so single-sided
    //     primitives are fully lit/shadowed).
    gpu::GraphicsPipelineDescription pipelineDesc{};
    pipelineDesc.vertexShader   = vs.get();
    pipelineDesc.fragmentShader = fs.get();
    pipelineDesc.layout         = graphicsLayout.get();
    pipelineDesc.topology       = gpu::PrimitiveTopology::TriangleList;
    pipelineDesc.colorFormat    = surface->colorFormat();
    pipelineDesc.depthFormat    = surface->depthFormat();
    pipelineDesc.cullMode       = gpu::CullMode::None;
    pipelineDesc.debugName      = "Ray query shadows pipeline";
    pipelineDesc.addVertexBufferDescription(gpu::MeshPN::vertexBufferDescription());
    auto pipeline = device->createGraphicsPipeline(pipelineDesc);
    cleanup.push(pipeline);

    // 11. Render loop.
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
                surface->resize({
                    static_cast<uint32_t>(event.window.data1),
                    static_cast<uint32_t>(event.window.data2)
                });
            }
        }

        const float t = static_cast<float>(SDL_GetTicks64()) / 1000.0f;

        // Orbit the point light above the objects so the shadows move.
        const float orbitRadius = 3.2f;
        LightUBO lightData{};
        lightData.lightPosition = glm::vec4(
            orbitRadius * std::cos(t),
            4.0f,
            orbitRadius * std::sin(t),
            1.0f
        );
        lightData.lightColor = glm::vec4(1.0f, 0.97f, 0.9f, 1.6f);
        lightData.ambient    = glm::vec4(0.12f, 0.12f, 0.14f, 1.0f);
        lightUbo->updateUniformBuffer(lightData);

        auto frame = surface->beginFrame();
        auto cmd   = graphicsQueue.createCommandBuffer("Frame command buffer");

        cmd->begin();

        // Rebuild the top-level acceleration structure before rendering.
        populateInstances();
        rayTracingScene->buildOrUpdate(cmd.get());

        cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
        cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
        cmd->beginRendering(frame.get());
        cmd->clearColor(0, gpu::Color(0.04f, 0.05f, 0.07f, 1.0f));
        cmd->clearDepth(1.0f);

        cmd->bindPipeline(pipeline.get());
        cmd->bindResourceSet(pipeline.get(), 0, cameraSet.get());
        cmd->bindResourceSet(pipeline.get(), 2, lightSet.get());

        for (auto& obj : objects)
        {
            cmd->bindResourceSet(pipeline.get(), 1, obj.objectSet.get());
            obj.mesh->draw(cmd.get());
        }

        cmd->endRendering();
        cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

        surface->present(cmd.get());
        cmd->end();
        graphicsQueue.submit(cmd.get());
        surface->endFrame(frame.get());
    }

    // 12. Cleanup.
    device->waitIdle();

    for (auto& obj : objects)
    {
        obj.objectSet->cleanup();
        obj.objectUbo->cleanup();
        for (auto& rtMesh : obj.rtMeshes) { rtMesh->cleanup(); }
        obj.mesh->cleanup();
    }
    objects.clear();

    cleanup.flush();

    surface->cleanup();
    device->cleanup();
    instance->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
