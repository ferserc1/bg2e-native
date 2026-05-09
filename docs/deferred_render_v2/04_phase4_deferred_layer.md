# Phase 4 — Deferred Layer

## Objective

Create the `DeferredLayer` class that implements a complete deferred pipeline: G-buffer generation from scene objects + PBR compositing pass. This is the core class of the deferred renderer. Each `DeferredLayer` renders only objects matching its layer type (opaque or transparent).

---

## Important: Build Policy

**DO NOT compile or build the project.** The user will personally review and test each implementation step.

---

## Sub-phases

### 4.1 — Create DeferredLayer Class

**Files:**
- `lib/include/bg2e/render/deferred/DeferredLayer.hpp`
- `lib/src/bg2e/render/deferred/DeferredLayer.cpp`

**Header structure:**
```
DeferredLayer.hpp
├── GPL license header
├── #pragma once
├── Includes:
│   ├── RenderLayer.hpp
│   ├── gbuffer/GBufferManager.hpp
│   ├── vulkan/factory/GraphicsPipeline.hpp
│   ├── vulkan/PipelineDataBinding.hpp
│   ├── scene/vk/FrameDataBinding.hpp
│   ├── scene/vk/ObjectDataBinding.hpp
│   ├── scene/vk/EnvironmentDataBinding.hpp
│   ├── scene/vk/LightDataBinding.hpp
│   ├── vulkan/rt/RayTracingSceneDataBinding.hpp
│   ├── scene/RenderQueueVisitor.hpp
│   ├── render/RenderQueue.hpp
│   └── vulkan/factory/Sampler.hpp
├── enum class LayerType { Opaque, Transparent }
├── class DeferredLayer : public RenderLayer
├── Public:
│   ├── DeferredLayer(Engine* engine, LayerType type)
│   ├── ~DeferredLayer()
│   ├── void build(VkExtent2D extent, VkFormat outputFormat) override
│   ├── void initFrameResources(vulkan::DescriptorSetAllocator* allocator) override
│   ├── void render(VkCommandBuffer cmd, uint32_t currentFrame,
│   │               const vulkan::Image* inputImage,
│   │               const vulkan::Image* outputImage,
│   │               vulkan::FrameResources& frameResources) override
│   ├── void resize(VkExtent2D newExtent) override
│   ├── void cleanup() override
│   ├── void setLightUniforms(const scene::vk::LightDataBinding::LightUniforms& lu)
│   └── void setRtDataBinding(vulkan::rt::RayTracingSceneDataBinding* rt)
├── Protected:
│   ├── LayerType _layerType
│   ├── std::unique_ptr<GBufferManager> _gbuffer
│   ├── VkPipeline _gbufferPipeline
│   ├── VkPipeline _compositePipeline
│   ├── VkPipelineLayout _gbufferPipelineLayout
│   ├── VkPipelineLayout _compositePipelineLayout
│   ├── VkDescriptorSetLayout _compositeGBufferDSLayout
│   ├── std::unique_ptr<scene::vk::FrameDataBinding> _frameDataBinding
│   ├── std::unique_ptr<scene::vk::ObjectDataBinding> _objectDataBinding
│   ├── std::unique_ptr<scene::vk::EnvironmentDataBinding> _environmentDataBinding
│   ├── std::unique_ptr<scene::vk::LightDataBinding> _lightDataBinding
│   ├── scene::vk::LightDataBinding::LightUniforms _lightUniforms
│   ├── vulkan::rt::RayTracingSceneDataBinding* _rtDataBinding  // non-owning
│   ├── scene::RenderQueueVisitor<scene::Drawable> _renderQueueVisitor
│   ├── render::RenderQueue<scene::Drawable> _renderQueue
│   ├── VkSampler _gbufferSampler
│   └── struct CompositePushConstants { gamma, brightness, contrast, exposure }
```

### 4.2 — Pipeline Creation: G-Buffer Pipeline

Created in `build()` using `vulkan::factory::GraphicsPipeline`:

```cpp
void DeferredLayer::build(VkExtent2D extent, VkFormat outputFormat) {
    RenderLayer::build(extent, outputFormat);

    // Create G-buffer manager
    _gbuffer = std::make_unique<GBufferManager>(_engine);
    _gbuffer->build(extent);

    // Create data bindings
    _frameDataBinding = std::make_unique<scene::vk::FrameDataBinding>(_engine);
    _objectDataBinding = std::make_unique<scene::vk::ObjectDataBinding>(_engine);
    _environmentDataBinding = std::make_unique<scene::vk::EnvironmentDataBinding>(_engine);
    _lightDataBinding = std::make_unique<scene::vk::LightDataBinding>(_engine);

    // Create G-buffer pipeline
    createGBufferPipeline();

    // Create composite pipeline
    createCompositePipeline();
}
```

**G-buffer pipeline configuration:**
- Vertex shader: `deferred_gbuffer.vert.spv`
- Fragment shader: `deferred_gbuffer.frag.spv`
- Input state: `scene::Drawable::bindingDescription()` / `attributeDescriptions()` (PNUT format)
- 4 color attachment formats: albedo (R8G8B8A8_UNORM), normal (R8G8B8A8_SNORM), material (R8G8B8A8_UNORM), position (R32G32B32A32_SFLOAT)
- Depth format: `VK_FORMAT_D32_SFLOAT`
- Depth test: enabled, depth write enabled, `VK_COMPARE_OP_LESS`
- Blending: disabled
- Cull mode: `VK_CULL_MODE_BACK_BIT`, front face: `VK_FRONT_FACE_COUNTER_CLOCKWISE`
- Multisample: disabled (`VK_SAMPLE_COUNT_1_BIT`)
- Topology: `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST`

**Pipeline layout:**
- set=0: `FrameDataBinding::layout()` (scene uniforms)
- set=1: `ObjectDataBinding::layout()` (object uniforms + textures)
- set=2: `EnvironmentDataBinding::layout()` (IBL textures)
- set=3: `LightDataBinding::layout()` (light uniforms)
- (optional set=4: RT TLAS)

### 4.3 — Pipeline Creation: Composite Pipeline

**Composite pipeline configuration:**
- Vertex shader: `deferred_composite.vert.spv`
- Fragment shader: `deferred_composite.frag.spv` (or `deferred_composite_rt.frag.spv` if RT supported)
- No vertex input (fullscreen quad generated procedurally in vertex shader)
- 1 color attachment format: output format (swapchain format)
- No depth test
- No blending
- No multisample
- Cull mode: `VK_CULL_MODE_BACK_BIT`

**Composite pipeline layout:**
- set=0: G-buffer descriptor set layout (5 bindings: 4 G-buffer textures + 1 input image, all COMBINED_IMAGE_SAMPLER)
- set=1: `FrameDataBinding::layout()` (scene uniforms)
- set=2: `EnvironmentDataBinding::layout()` (IBL textures)
- set=3: `LightDataBinding::layout()` (light uniforms)
- set=4: RT TLAS descriptor set layout (optional, only if RT supported)
- Push constant range: `CompositePushConstants` (16 bytes), `VK_SHADER_STAGE_FRAGMENT_BIT`

### 4.4 — G-Buffer Generation Pass

The G-buffer pass renders scene geometry into the 4 G-buffer images.

**`render()` sub-step: G-buffer pass**

At the start of `render()`, extract camera data from `_scene`:
```cpp
auto mainCamera = _scene->mainCamera();
auto viewMatrix = mainCamera->ownerNode()->invertedWorldMatrix();
auto projMatrix = mainCamera->projectionMatrix();
auto cameraWorldPos = mainCamera->ownerNode()->worldPosition();
```

Then proceed with the G-buffer pass:
```cpp
// 1. Transition G-buffers to attachment layout
_gbuffer->transitionToAttachment(cmd);

// 2. Clear G-buffers
_gbuffer->clear(cmd);

// 3. Begin dynamic rendering with 4 color attachments + depth
vulkan::macros::cmdClearImagesAndBeginRendering(cmd,
    _gbuffer->images(),
    { {0, 0, 0, 0} },
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    _gbuffer->depthImage().get(),
    1.0f);

// 4. Set viewport/scissor
vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, _extent);

// 5. Bind G-buffer pipeline
vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gbufferPipeline);

// 6. Create descriptor sets (using members: _scene, _environment, _lightUniforms)
auto sceneDS = _frameDataBinding->newDescriptorSet(frameResources, viewMatrix, projMatrix);
auto envDS = _environmentDataBinding->newDescriptorSet(frameResources, _environment);
auto lightDS = _lightDataBinding->newDescriptorSet(frameResources, _lightUniforms);

// 7. Build render queue from scene (filter by layer type)
_renderQueue.beginFrame();
_renderQueueVisitor.enqueue(_scene->rootNode(), &_renderQueue);

// 8. Create descriptor set function
auto dsFunction = [&](scene::MaterialBase* mat, const glm::mat4& transform) {
    auto objectDS = _objectDataBinding->newDescriptorSet(frameResources, mat, transform);
    return std::vector<VkDescriptorSet>{ sceneDS, objectDS, envDS, lightDS };
};

// 9. Render queue items based on layer type
if (_layerType == LayerType::Opaque) {
    _renderQueue.render(RenderQueueType::Opaque, cmd, _gbufferPipelineLayout,
                        dsFunction, cameraWorldPos);
} else {
    _renderQueue.render(RenderQueueType::Transparent, cmd, _gbufferPipelineLayout,
                        dsFunction, cameraWorldPos);
    _renderQueue.render(RenderQueueType::SolidTransparent, cmd, _gbufferPipelineLayout,
                        dsFunction, cameraWorldPos);
}

// 10. End rendering
vulkan::cmdEndRendering(cmd);
```

### 4.5 — G-Buffer Vertex Shader

**File:** `shaders/src/deferred_gbuffer.vert.glsl`

```glsl
#version 450
#extension GL_ARB_shading_language_include: require
#include "lib/constants.glsl"
#include "lib/normal_map.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec2 inUV1;
layout(location = 4) in vec3 inTangent;

layout(set = 0, binding = 0) uniform SceneData {
    mat4 viewMatrix;
    mat4 projMatrix;
};

layout(set = 1, binding = 0) uniform PBRObjectData {
    mat4 modelMatrix;
    // ... PBRMaterialData follows
};

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV0;
layout(location = 3) out vec2 outUV1;
layout(location = 4) out mat3 outTBN;

void main() {
    vec4 worldPos = modelMatrix * vec4(inPosition, 1.0);
    outWorldPos = worldPos.xyz;
    outNormal = normalize(mat3(modelMatrix) * inNormal);
    outUV0 = inUV0;
    outUV1 = inUV1;
    outTBN = TBNMatrix(modelMatrix, inNormal, inTangent);
    gl_Position = projMatrix * viewMatrix * worldPos;
}
```

> **IMPORTANT:** Any questions about shader implementation should be asked. This vertex shader follows the same structure as `basic_forward.vert.glsl`. The existing shader library (`lib/normal_map.glsl`, `lib/uniforms.glsl`) should be used for all operations.

### 4.6 — G-Buffer Fragment Shader

**File:** `shaders/src/deferred_gbuffer.frag.glsl`

```glsl
#version 450
#extension GL_ARB_shading_language_include: require
#include "lib/constants.glsl"
#include "lib/uniforms.glsl"
#include "lib/normal_map.glsl"

// Uniforms (set=1, same as forward renderer)
layout(set = 1, binding = 0) uniform PBRObjectData {
    mat4 modelMatrix;
    PBRMaterialData material;
};

// Texture samplers (set=1)
layout(set = 1, binding = 1) uniform sampler2D albedoTex;
layout(set = 1, binding = 2) uniform sampler2D normalTex;
layout(set = 1, binding = 3) uniform sampler2D metallicTex;
layout(set = 1, binding = 4) uniform sampler2D roughnessTex;
layout(set = 1, binding = 5) uniform sampler2D aoTex;

// G-buffer outputs
layout(location = 0) out vec4 g_Albedo;
layout(location = 1) out vec4 g_Normal;
layout(location = 2) out vec4 g_Material;
layout(location = 3) out vec4 g_Position;

// Inputs from vertex shader
layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec2 inUV1;
layout(location = 4) in mat3 inTBN;

void main() {
    // Sample albedo (sRGB to linear using existing helper)
    vec4 albedo = sampleAlbedo(albedoTex, inUV0, inUV1, material, 2.2);
    g_Albedo = albedo;

    // Sample and transform normal to world space
    vec3 normal = sampleNormal(normalTex, inUV0, inUV1, material, inTBN);
    g_Normal = vec4(normal * 0.5 + 0.5, 1.0);  // Map [-1,1] to [0,1]

    // Material properties
    float metallic = sampleMetallic(metallicTex, inUV0, inUV1, material);
    float roughness = sampleRoughness(roughnessTex, inUV0, inUV1, material);
    float ao = sampleAmbientOcclussion(aoTex, inUV0, inUV1, material);
    float sheen = material.sheenIntensity;
    g_Material = vec4(metallic, roughness, ao, sheen);

    // World-space position
    g_Position = vec4(inWorldPos, 1.0);
}
```

> **IMPORTANT:** Any questions about shader implementation should be asked. The shader reuses `lib/uniforms.glsl` texture sampling functions (`sampleAlbedo`, `sampleNormal`, `sampleMetallic`, `sampleRoughness`, `sampleAmbientOcclussion`). The existing shader library should be used for all operations.

### 4.7 — Compositing Pass

The compositing pass renders a fullscreen quad that reads G-buffers and computes PBR lighting.

**`render()` sub-step: Compositing pass**
```cpp
// 1. Transition G-buffers to shader read
_gbuffer->transitionToShaderRead(cmd);

// 2. Clear output image and begin rendering
vulkan::macros::cmdClearImageAndBeginRendering(cmd, outputImage,
    { {0, 0, 0, 0} }, VK_IMAGE_LAYOUT_UNDEFINED);
vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, _extent);

// 3. Bind composite pipeline
vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _compositePipeline);

// 4. Create G-buffer descriptor set
auto gbufferDS = frameResources.newDescriptorSet(_compositeGBufferDSLayout);
// Update with G-buffer images + input image
// ... (see descriptor set creation below)

// 5. Create other descriptor sets (using members: _scene, _environment, _lightUniforms)
auto sceneDS = _frameDataBinding->newDescriptorSet(frameResources, viewMatrix, projMatrix);
auto envDS = _environmentDataBinding->newDescriptorSet(frameResources, _environment);
auto lightDS = _lightDataBinding->newDescriptorSet(frameResources, _lightUniforms);

// 6. Bind descriptor sets
vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    _compositePipelineLayout, 0, 1, &gbufferDS, 0, nullptr);
vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    _compositePipelineLayout, 1, 1, &sceneDS, 0, nullptr);
vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    _compositePipelineLayout, 2, 1, &envDS, 0, nullptr);
vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    _compositePipelineLayout, 3, 1, &lightDS, 0, nullptr);

// 7. Push constants
CompositePushConstants pc{ 2.2f, _brightness, _contrast, _exposure };
vkCmdPushConstants(cmd, _compositePipelineLayout,
    VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CompositePushConstants), &pc);

// 8. Draw fullscreen quad (6 vertices, 2 triangles)
vkCmdDraw(cmd, 6, 1, 0, 0);

// 9. End rendering
vulkan::cmdEndRendering(cmd);
```

### 4.8 — Compositing Vertex Shader

**File:** `shaders/src/deferred_composite.vert.glsl`

```glsl
#version 450

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out vec2 vTexcoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexcoord = aTexCoord;
}
```

This shader uses a fullscreen quad generated procedurally. The vertex data is created in the C++ code (same pattern as `ColorAttachmentsCanvas`).

> **IMPORTANT:** Any questions about shader implementation should be asked.

### 4.9 — Compositing Fragment Shader

**File:** `shaders/src/deferred_composite.frag.glsl`

```glsl
#version 450
#extension GL_ARB_shading_language_include: require
#include "lib/constants.glsl"
#include "lib/uniforms.glsl"
#include "lib/pbr.glsl"
#include "lib/color_correction.glsl"

// G-buffer samplers (set=0)
layout(set = 0, binding = 0) uniform sampler2D g_Albedo;
layout(set = 0, binding = 1) uniform sampler2D g_Normal;
layout(set = 0, binding = 2) uniform sampler2D g_Material;
layout(set = 0, binding = 3) uniform sampler2D g_Position;
layout(set = 0, binding = 4) uniform sampler2D g_InputImage;

// Scene data (set=1)
layout(set = 1, binding = 0) uniform SceneData {
    mat4 viewMatrix;
    mat4 projMatrix;
};

// Environment data (set=2)
layout(set = 2, binding = 0) uniform samplerCube irradianceMap;
layout(set = 2, binding = 1) uniform samplerCube prefilteredEnvMap;
layout(set = 2, binding = 2) uniform sampler2D brdfLUT;
layout(set = 2, binding = 3) uniform EnvironmentData {
    float maxReflectionLOD;
};

// Light data (set=3)
layout(set = 3, binding = 0) uniform LightData {
    Light lights[LIGHT_COUNT];
    uint lightCount;
};

// Push constants
layout(push_constant) uniform PC {
    float gamma;
    float brightness;
    float contrast;
    float exposure;
};

layout(location = 0) in vec2 vTexcoord;
layout(location = 0) out vec4 outColor;

void main() {
    // Sample G-buffers
    vec4 albedo = texture(g_Albedo, vTexcoord);
    vec3 normal = texture(g_Normal, vTexcoord).xyz * 2.0 - 1.0;
    vec4 materialData = texture(g_Material, vTexcoord);
    vec3 worldPos = texture(g_Position, vTexcoord).xyz;
    vec4 inputColor = texture(g_InputImage, vTexcoord);

    float metallic = materialData.r;
    float roughness = max(materialData.g, 0.05);
    float ao = materialData.b;
    float sheenIntensity = materialData.a;

    // Camera position from inverse view matrix
    vec3 cameraPos = vec3(inverse(viewMatrix)[3]);
    vec3 viewDir = normalize(cameraPos - worldPos);

    // F0 for Fresnel
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);

    // Sheen color (simplified - use albedo as sheen tint)
    vec3 sheenColor = albedo.rgb;

    // Direct lighting loop
    vec3 Lo = vec3(0.0);
    for (uint i = 0; i < lightCount; i++) {
        if (lights[i].type == LIGHT_TYPE_DISABLED) continue;
        Lo += calcRadiance(lights[i], viewDir, worldPos, metallic, roughness,
                          F0, normal, albedo.rgb, sheenIntensity, sheenColor, ao);
    }

    // Ambient/IBL lighting
    vec3 ambient = calcAmbientLight(viewDir, normal, F0, albedo.rgb, metallic, roughness,
                                    irradianceMap, prefilteredEnvMap, maxReflectionLOD,
                                    brdfLUT, ao, sheenIntensity, sheenColor);

    vec3 color = ambient + Lo;

    // Blend with input image (previous layer)
    // For opaque layer: opaque result replaces background where alpha > 0
    // For transparent layer: transparent result blends over opaque
    vec3 finalColor = mix(inputColor.rgb, color, albedo.a);

    // Color correction
    finalColor = exposure(finalColor, pushConstant.exposure);
    outColor = lineal2SRGB(vec4(finalColor, 1.0), pushConstant.gamma);
    outColor = brightnessContrast(outColor, pushConstant.brightness, pushConstant.contrast);
}
```

> **IMPORTANT:** Any questions about shader implementation should be asked. The shader uses the existing `lib/pbr.glsl` library for all PBR calculations (`calcRadiance`, `calcAmbientLight`). The blending strategy between layers needs careful consideration. The existing shader library should be used for all operations.

### 4.10 — Compositing Fragment Shader with RT Shadows

**File:** `shaders/src/deferred_composite_rt.frag.glsl`

Same as the non-RT version but with ray query shadow testing in the lighting loop:

```glsl
#version 460
#extension GL_EXT_ray_query : require
#extension GL_ARB_shading_language_include : require
#include "lib/constants.glsl"
#include "lib/uniforms.glsl"
#include "lib/pbr.glsl"
#include "lib/color_correction.glsl"

// ... same bindings as non-RT version, plus:
layout(set = 4, binding = 0) uniform accelerationStructureEXT tlas;

// ... same push constants and inputs ...

void main() {
    // ... same G-buffer sampling and setup as non-RT version ...

    // Direct lighting with shadow testing
    vec3 Lo = vec3(0.0);
    for (uint i = 0; i < lightCount; i++) {
        if (lights[i].type == LIGHT_TYPE_DISABLED) continue;

        // Compute light direction and max distance
        vec3 toLight;
        float tMax;
        if (lights[i].type == LIGHT_TYPE_POINT) {
            toLight = lights[i].position - worldPos;
            tMax = length(toLight);
            toLight = normalize(toLight);
        } else if (lights[i].type == LIGHT_TYPE_DIRECTIONAL) {
            toLight = -normalize(lights[i].direction);
            tMax = 1000000.0;
        } else { // SPOT
            toLight = lights[i].position - worldPos;
            tMax = length(toLight);
            toLight = normalize(toLight);
        }

        // Shadow ray query
        rayQueryEXT rq;
        rayQueryInitializeEXT(rq, tlas,
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
            0xFF, worldPos + normal * 0.01, 0.0, toLight, tMax);
        while (rayQueryProceedEXT(rq)) {}

        if (rayQueryGetIntersectionTypeEXT(rq, true) ==
            gl_RayQueryCommittedIntersectionNoneEXT) {
            // No shadow — add light contribution
            Lo += calcRadiance(lights[i], viewDir, worldPos, metallic, roughness,
                              F0, normal, albedo.rgb, sheenIntensity, sheenColor, ao);
        }
    }

    // ... same ambient + color correction as non-RT version ...
}
```

> **IMPORTANT:** Any questions about shader implementation should be asked. The RT shadow testing follows the same pattern as `basic_forward_rt_shadows.frag.glsl`. The existing shader library should be used for all operations.

### 4.11 — Fullscreen Quad Vertex Buffer

Create the fullscreen quad vertex buffer in `build()` (same pattern as `ColorAttachmentsCanvas`):

```cpp
struct QuadVertex {
    glm::vec2 pos;
    glm::vec2 uv;
};

// Fullscreen quad vertices (two triangles)
std::vector<QuadVertex> quadVertices = {
    {{-1.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 1.0f,  1.0f}, {1.0f, 1.0f}},
    {{-1.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 1.0f,  1.0f}, {1.0f, 1.0f}},
    {{-1.0f,  1.0f}, {0.0f, 1.0f}},
};
```

This buffer is uploaded to a `vulkan::Buffer` and bound before drawing the fullscreen quad.

### 4.12 — Render Queue Filtering

The `DeferredLayer` filters objects by layer type when rendering:
- `LayerType::Opaque`: Renders `RenderQueueType::Opaque` items only
- `LayerType::Transparent`: Renders `RenderQueueType::Transparent` and `RenderQueueType::SolidTransparent` items

This uses the existing `MaterialAttributes::isTransparent()` and `isSolid()` properties, which are already used by `RenderQueue::enqueue()` to route items to the correct sub-queue.

### 4.13 — Code Review Checklist

- [ ] `DeferredLayer` inherits from `RenderLayer` and overrides all virtual methods
- [ ] Camera matrices and position are extracted from `_scene->mainCamera()` inside `render()`
- [ ] `setLightUniforms()` and `setRtDataBinding()` configure per-frame data before `render()`
- [ ] G-buffer pipeline creates 4 color attachments with correct formats
- [ ] G-buffer pipeline uses `deferred_gbuffer.vert.spv` and `deferred_gbuffer.frag.spv`
- [ ] Composite pipeline uses `deferred_composite.vert.spv` and `deferred_composite.frag.spv`
- [ ] RT variant selected conditionally based on `engine->rayTracingSupported()`
- [ ] G-buffer fragment shader writes correct data to all 4 attachments
- [ ] Compositing fragment shader reads G-buffers and computes PBR lighting using `lib/pbr.glsl`
- [ ] Layer type filtering works (opaque vs transparent objects)
- [ ] Input image blending works correctly (`mix(inputColor, gbufferColor, alpha)`)
- [ ] Push constants are passed correctly
- [ ] Descriptor sets are created and bound correctly
- [ ] Fullscreen quad vertex buffer is created correctly
- [ ] Resize recreates G-buffers and pipelines
- [ ] Cleanup releases all Vulkan resources

---

## Existing Code References

- `lib/include/bg2e/render/RendererBasicForward.hpp` — pipeline creation pattern
- `lib/src/bg2e/render/RendererBasicForward.cpp` — draw() implementation, descriptor set creation
- `lib/include/bg2e/render/ColorAttachmentsCanvas.hpp` — fullscreen quad rendering pattern
- `lib/src/bg2e/render/ColorAttachmentsCanvas.cpp` — descriptor set creation for sampled images
- `lib/include/bg2e/render/vulkan/factory/GraphicsPipeline.hpp` — pipeline factory API
- `lib/include/bg2e/scene/vk/FrameDataBinding.hpp` — scene uniform binding
- `lib/include/bg2e/scene/vk/ObjectDataBinding.hpp` — object uniform + texture binding
- `lib/include/bg2e/scene/vk/EnvironmentDataBinding.hpp` — IBL texture binding
- `lib/include/bg2e/scene/vk/LightDataBinding.hpp` — light uniform binding
- `lib/include/bg2e/render/RenderQueue.hpp` — render queue system
- `lib/include/bg2e/scene/RenderQueueVisitor.hpp` — scene traversal for render queue
- `shaders/src/basic_forward.vert.glsl` — vertex shader structure to follow
- `shaders/src/basic_forward.frag.glsl` — fragment shader structure to follow
- `shaders/src/lib/pbr.glsl` — PBR helper functions
- `shaders/src/lib/uniforms.glsl` — texture sampling helpers
- `shaders/src/lib/normal_map.glsl` — TBN matrix computation
- `shaders/src/lib/color_correction.glsl` — tone mapping and color correction
