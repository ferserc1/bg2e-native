# Step 09: Validation Example — Cornell Box

## Goal

Create `examples/gpu/13_ray_tracing_pipeline` with a Cornell-box-style scene, progressive refinement, and complete RT pipeline validation.

## Files to Create

### `examples/gpu/13_ray_tracing_pipeline/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.18)

set(APP_TARGET_NAME gpu_ray_tracing_pipeline)
set(APP_SHADERS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(APP_SHADERS_DST "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders")
set(METAL_SHADERS_DST "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders/metal")

bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME})

compile_shaders_shaderlib(${APP_TARGET_NAME} ${VULKAN_SDK}
    "${APP_SHADERS_SRC}" "${APP_SHADERS_DST}")
bundle_resources(TARGET_NAME ${APP_TARGET_NAME}
    SRC_PATH ${APP_SHADERS_DST}
    SUBPATH "shaders/${APP_TARGET_NAME}")

if(APPLE)
    compile_metal_shaders(${APP_TARGET_NAME}
        "${APP_SHADERS_SRC}" "${METAL_SHADERS_DST}")
    bundle_resources(TARGET_NAME ${APP_TARGET_NAME}
        SRC_PATH ${METAL_SHADERS_DST}
        SUBPATH "shaders/${APP_TARGET_NAME}")
endif()
```

### `examples/gpu/13_ray_tracing_pipeline/shaders/ray_tracing_pipeline.rgen.glsl`

Vulkan ray generation shader:

```glsl
#version 460
#extension GL_EXT_ray_tracing : enable

layout(set = 0, binding = 0, rgba32f) uniform image2D outputImage;
layout(set = 0, binding = 1) uniform CameraData {
    mat4 viewInverse;
    mat4 projInverse;
    vec4 misc; // x=frameCount, y=width, z=height
} cam;
layout(set = 1, binding = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadEXT vec3 hitValue;
hitAttributeEXT vec2 attribs;

struct HitData {
    vec4 color;
    vec4 emission;
    vec4 params; // x=roughness
};

layout(set = 2, binding = 0) buffer HitDataBuffer { HitData data[]; } hitData;

// Simple PCG random
uint pcgRandom(inout uint state) {
    state = state * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float randomFloat(inout uint state) {
    return float(pcgRandom(state)) / 4294967295.0;
}

vec3 cosineRandomDirection(vec2 uv, inout uint state) {
    float r1 = randomFloat(state);
    float r2 = randomFloat(state);
    float phi = 2.0 * 3.14159265 * r1;
    float cosTheta = sqrt(1.0 - r2);
    float sinTheta = sqrt(r2);
    return vec3(cos(phi) * sinTheta, cosTheta, sin(phi) * sinTheta);
}

void main()
{
    uint seed = gl_LaunchIDEXT.x + gl_LaunchSizeEXT.x * gl_LaunchIDEXT.y
              + uint(cam.misc.x) * 7919u;

    vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    vec2 inUV = pixelCenter / vec2(cam.misc.y, cam.misc.z);
    vec2 d = inUV * 2.0 - 1.0;

    vec4 origin = cam.viewInverse * vec4(0, 0, 0, 1);
    vec4 target = cam.projInverse * vec4(d.x, d.y, 1, 1);
    vec4 direction = cam.viewInverse * vec4(normalize(target.xyz), 0);

    float tmin = 0.001;
    float tmax = 10000.0;

    hitValue = vec3(0.0);
    traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xff,
        0, 0, 0,
        origin.xyz, tmin, direction.xyz, tmax, 0);

    // Progressive accumulation
    vec3 prevColor = imageLoad(outputImage, ivec2(gl_LaunchIDEXT.xy)).rgb;
    float frameCount = cam.misc.x;
    vec3 newColor;
    if (frameCount <= 1.0) {
        newColor = hitValue;
    } else {
        newColor = mix(prevColor, hitValue, 1.0 / frameCount);
    }
    imageStore(outputImage, ivec2(gl_LaunchIDEXT.xy), vec4(newColor, 1.0));
}
```

### `examples/gpu/13_ray_tracing_pipeline/shaders/ray_tracing_pipeline.rmiss.glsl`

Vulkan miss shader:

```glsl
#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;

void main()
{
    // Dark background — sky gradient
    hitValue = vec3(0.05, 0.05, 0.08);
}
```

### `examples/gpu/13_ray_tracing_pipeline/shaders/ray_tracing_pipeline.rchit.glsl`

Vulkan closest hit shader:

```glsl
#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 attribs;

struct HitData {
    vec4 color;
    vec4 emission;
    vec4 params;
};

layout(set = 2, binding = 0) buffer HitDataBuffer { HitData data[]; } hitData;

void main()
{
    HitData hd = hitData.data[gl_PrimitiveID];

    // Simple Lambert shading with a fixed light direction
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 normal = vec3(0.0, 1.0, 0.0); // simplified — all normals up
    float ndotl = max(dot(normal, lightDir), 0.0);

    hitValue = hd.color.rgb * (0.2 + 0.8 * ndotl) + hd.emission.rgb;
}
```

### `examples/gpu/13_ray_tracing_pipeline/shaders/ray_tracing_pipeline.rgen.metal`

Metal compute-based ray tracing kernel:

```metal
#include <metal_stdlib>
using namespace metal;

struct CameraData {
    float4x4 viewInverse;
    float4x4 projInverse;
    float4 misc; // x=frameCount, y=width, z=height
};

struct HitData {
    float4 color;
    float4 emission;
    float4 params;
};

kernel void rgenMain(
    texture2d<float, access::read_write> outputImage [[texture(0)]],
    device const CameraData& cam [[buffer(1)]],
    instance_acceleration_structure tlas [[buffer(2)]],
    device const HitData* hitData [[buffer(3)]],
    uint2 gid [[thread_position_in_grid]])
{
    if (gid.x >= uint(cam.misc.y) || gid.y >= uint(cam.misc.z)) return;

    float2 pixelCenter = float2(gid) + float2(0.5);
    float2 inUV = pixelCenter / float2(cam.misc.y, cam.misc.z);
    float2 d = inUV * 2.0 - 1.0;

    float4 origin = cam.viewInverse * float4(0, 0, 0, 1);
    float4 target = cam.projInverse * float4(d.x, d.y, 1, 1);
    float4 direction = cam.viewInverse * float4(normalize(target.xyz), 0);

    ray ray;
    ray.origin = origin.xyz;
    ray.direction = direction.xyz;
    ray.min_distance = 0.001;
    ray.max_distance = 10000.0;

    intersector<triangle_data> intersector;
    auto intersection = intersector.intersect(ray, tlas);

    float3 color;
    if (intersection.type == intersection_type::none) {
        color = float3(0.05, 0.05, 0.08);
    } else {
        uint primID = intersection.primitive_id;
        HitData hd = hitData[primID];
        float3 lightDir = normalize(float3(0.5, 1.0, 0.3));
        float3 normal = float3(0, 1, 0);
        float ndotl = max(dot(normal, lightDir), 0.0);
        color = hd.color.rgb * (0.2 + 0.8 * ndotl) + hd.emission.rgb;
    }

    // Progressive accumulation
    float3 prevColor = outputImage.read(gid).rgb;
    float frameCount = cam.misc.x;
    float3 newColor;
    if (frameCount <= 1.0) {
        newColor = color;
    } else {
        newColor = mix(prevColor, color, 1.0 / frameCount);
    }
    outputImage.write(float4(newColor, 1.0), gid);
}
```

### `examples/gpu/13_ray_tracing_pipeline/src/main.cpp`

Full example (key sections):

```cpp
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

struct HitData {
    glm::vec4 color;
    glm::vec4 emission;
    glm::vec4 params;
};

int main(int argc, char** argv)
{
    using namespace bg2e;

    // 1. Backend selection + window + instance + surface + device
    //    (same pattern as example 11)
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
        "GPU Ray Tracing Pipeline Example",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        windowFlags | SDL_WINDOW_RESIZABLE
    );

    auto* instance = backend->sharedInstance();
    instance->enableDebugMode(true);
    instance->create(window);

    std::shared_ptr<gpu::Surface> surface = backend->createWindowSurface(instance);
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    if (!physicalDevice->properties()->rayTracingSupported())
    {
        std::cerr << "Ray tracing not supported" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());
    gpu::CleanupManager cleanup(surface.get());

    // 2. Load shaders
    auto shaderLib = backend->createShaderLib(
        base::PlatformTools::shaderPath() / "gpu_ray_tracing_pipeline");
    auto rgen = shaderLib->rayGeneration("ray_tracing_pipeline", device.get());
    auto rmiss = shaderLib->miss("ray_tracing_pipeline", device.get());
    auto rchit = shaderLib->closestHit("ray_tracing_pipeline", device.get());
    // Metal: rmiss and rchit may be nullptr — this is expected
    if (rgen) cleanup.push(rgen);
    if (rmiss) cleanup.push(rmiss);
    if (rchit) cleanup.push(rchit);

    // 3. Pipeline layout
    //    set 0: output image (storage) + camera UBO
    //    set 1: acceleration structure
    //    set 2: hit data SSBO
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

    // 4. Create output storage image
    auto outputImage = device->createImage({
        {800, 600}, gpu::PixelFormat::R32G32B32A32_SFLOAT,
        gpu::ImageUsage::Storage | gpu::ImageUsage::TransferSrc,
        gpu::ImageType::Image2D, 1, 0, "RT output image"
    });
    cleanup.push(outputImage);

    // 5. Create camera UBO
    float aspect = 800.0f / 600.0f;
    auto projection = glm::perspective(glm::radians(40.0f), aspect, 0.1f, 100.0f);
    auto view = glm::lookAt(
        glm::vec3(0.0f, 2.7f, 5.0f),  // camera position
        glm::vec3(0.0f, 2.7f, 0.0f),  // look-at center
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    CameraData camData{};
    camData.viewInverse = glm::inverse(view);
    camData.projInverse = glm::inverse(projection);
    camData.misc = glm::vec4(1.0f, 800.0f, 600.0f, 0.0f);

    auto cameraUbo = device->createBuffer("Camera UBO");
    cameraUbo->createUniformBuffer(camData);
    cleanup.push(cameraUbo);

    // 6. Build Cornell box geometry
    //    Floor (white), ceiling (white), back wall (white),
    //    left wall (red), right wall (green),
    //    light rectangle (emissive), cube (matte), sphere (matte)
    struct ScenePiece {
        std::shared_ptr<gpu::MeshP> mesh;
        glm::mat4 model;
        HitData hitData;
        std::vector<std::shared_ptr<gpu::RayTracingMesh>> rtMeshes;
    };

    std::vector<ScenePiece> pieces;
    std::vector<HitData> allHitData;

    auto addPiece = [&](bg2e::geo::MeshP* data, const glm::mat4& model,
                        const glm::vec3& color, const glm::vec3& emission = glm::vec3(0.0f))
    {
        ScenePiece piece;
        piece.mesh = std::make_shared<gpu::MeshP>();
        piece.mesh->setMeshData(*data);
        piece.mesh->build(device.get());
        piece.model = model;
        piece.hitData.color = glm::vec4(color, 1.0f);
        piece.hitData.emission = glm::vec4(emission, 1.0f);
        piece.hitData.params = glm::vec4(0.0f);

        for (uint32_t s = 0; s < piece.mesh->submeshCount(); ++s)
        {
            piece.rtMeshes.push_back(
                device->createRayTracingMesh(piece.mesh->rayTracingMeshDescription(s))
            );
        }

        pieces.push_back(std::move(piece));
        delete data;
    };

    // Cornell box walls
    addPiece(bg2e::geo::createPlanePN(5.5f, 5.5f),
             glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
             glm::vec3(0.73f, 0.73f, 0.73f)); // floor

    addPiece(bg2e::geo::createPlanePN(5.5f, 5.5f),
             glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.5f, 0.0f)),
                         glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
             glm::vec3(0.73f, 0.73f, 0.73f)); // ceiling

    addPiece(bg2e::geo::createPlanePN(5.5f, 5.5f),
             glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.75f, -2.75f)),
                         glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
             glm::vec3(0.73f, 0.73f, 0.73f)); // back wall

    addPiece(bg2e::geo::createPlanePN(5.5f, 5.5f),
             glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(-2.75f, 2.75f, 0.0f)),
                         glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
             glm::vec3(0.63f, 0.06f, 0.06f)); // left wall (red)

    addPiece(bg2e::geo::createPlanePN(5.5f, 5.5f),
             glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(2.75f, 2.75f, 0.0f)),
                         glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
             glm::vec3(0.12f, 0.45f, 0.15f)); // right wall (green)

    // Light rectangle at ceiling
    addPiece(bg2e::geo::createPlanePN(1.3f, 1.3f),
             glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.49f, 0.0f)),
             glm::vec3(0.0f), glm::vec3(8.0f, 8.0f, 8.0f)); // emissive light

    // Cube
    addPiece(bg2e::geo::createCubePN(1.2f, 1.8f, 1.2f),
             glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.9f, 0.5f)),
             glm::vec3(0.73f, 0.73f, 0.73f));

    // Sphere
    addPiece(bg2e::geo::createSpherePN(0.6f, 32, 24),
             glm::translate(glm::mat4(1.0f), glm::vec3(1.2f, 0.6f, -0.5f)),
             glm::vec3(0.73f, 0.73f, 0.73f));

    // 7. Build BLAS
    device->immediateSubmit([&](gpu::CommandBuffer* cmd)
    {
        for (auto& piece : pieces)
        {
            for (auto& rtMesh : piece.rtMeshes)
            {
                cmd->buildRayTracingMesh(rtMesh.get());
            }
        }
    });

    // 8. Create TLAS
    auto rayTracingScene = device->createRayTracingScene("Cornell box scene");
    cleanup.push(rayTracingScene);

    uint32_t instanceId = 0;
    for (auto& piece : pieces)
    {
        for (auto& rtMesh : piece.rtMeshes)
        {
            rayTracingScene->addInstance(rtMesh.get(), piece.model, instanceId++, 0xFF);
        }
    }

    device->immediateSubmit([&](gpu::CommandBuffer* cmd)
    {
        rayTracingScene->buildOrUpdate(cmd);
    });

    // 9. Create hit data SSBO
    for (auto& piece : pieces)
    {
        allHitData.push_back(piece.hitData);
    }

    auto hitDataBuffer = device->createBuffer("Hit Data SSBO");
    hitDataBuffer->createStorageBuffer(allHitData.data(),
        allHitData.size() * sizeof(HitData));
    cleanup.push(hitDataBuffer);

    // 10. Create resource sets
    auto outputSet = device->createResourceSet(layout.get(), 0, "Output set");
    outputSet->setStorageImage({.vulkan = 0, .metal = 0}, outputImage.get());
    outputSet->setUniformBuffer({.vulkan = 1, .metal = 1}, cameraUbo.get());
    outputSet->update();
    cleanup.push(outputSet);

    auto sceneSet = device->createResourceSet(layout.get(), 1, "Scene set");
    sceneSet->setRayTracingScene({.vulkan = 0, .metal = 2}, rayTracingScene.get());
    sceneSet->update();
    cleanup.push(sceneSet);

    auto dataSet = device->createResourceSet(layout.get(), 2, "Data set");
    dataSet->setStorageBuffer({.vulkan = 0, .metal = 3}, hitDataBuffer.get());
    dataSet->update();
    cleanup.push(dataSet);

    // 11. Create RayTracingPipeline
    gpu::RayTracingPipelineDescription pipelineDesc{};
    pipelineDesc.raygenShader = rgen.get();
    pipelineDesc.missShader = rmiss.get();     // may be nullptr on Metal
    pipelineDesc.closestHitShader = rchit.get(); // may be nullptr on Metal
    pipelineDesc.layout = layout.get();
    pipelineDesc.maxRecursionDepth = 1;
    pipelineDesc.debugName = "Cornell box RT pipeline";
    auto rtPipeline = device->createRayTracingPipeline(pipelineDesc);
    cleanup.push(rtPipeline);

    // 12. Render loop
    auto& graphicsQueue = device->graphicsQueue();
    uint32_t frameCount = 0;
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
                // Recreate output image for new size
                // (omitted for brevity — same pattern as example 08)
            }
        }

        frameCount++;
        camData.misc.x = static_cast<float>(frameCount);
        cameraUbo->updateUniformBuffer(camData);

        auto frame = surface->beginFrame();
        auto cmd = graphicsQueue.createCommandBuffer("Frame");

        cmd->begin();

        // Trace rays into output image
        cmd->transition(outputImage.get(), gpu::ImageLayout::General);
        cmd->bindPipeline(rtPipeline.get());
        cmd->bindResourceSet(rtPipeline.get(), 0, outputSet.get());
        cmd->bindResourceSet(rtPipeline.get(), 1, sceneSet.get());
        cmd->bindResourceSet(rtPipeline.get(), 2, dataSet.get());
        cmd->traceRays(800, 600);

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
        for (auto& rtMesh : piece.rtMeshes) { rtMesh->cleanup(); }
        piece.mesh->cleanup();
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
```

## Integration Points

- Follows the same structure as example 11 (`11_ray_query_shadows`) for window creation, backend selection, and device setup.
- Uses `bg2e::geo` primitives for Cornell box geometry.
- Uses `gpu::MeshP` for position-only geometry (no normals needed for RT).
- Uses `FrameResourceRing` pattern is NOT needed — the camera UBO is updated via `updateUniformBuffer()` directly.
- Uses `copyImage()` to transfer the ray-traced result to the swapchain.

## Verification

Example compiles and runs. Cornell box renders progressively. Image refines over time.
