# Phase 2 — DeferredCompositor (Compositing Pass)

## Objective

Create a class that manages the compositing pipeline: reading G-buffers and calculating per-pixel lighting. It will include graphics pipeline management, GLSL shaders for full-screen quads, and light calculation (with/without ray tracing).

Design inspired by `ColorAttachmentsCanvas` but with full PBR lighting logic.

---

## New Files to Create

### A) `lib/include/bg2e/render/DeferredCompositor.hpp`
### B) `lib/src/bg2e/render/DeferredCompositor.cpp`
### C) `shaders/src/deferred_lighting.vert.glsl`
### D) `shaders/src/deferred_lighting.frag.glsl`

---

## A) DeferredCompositor.hpp — Structure and API

```
lib/include/bg2e/render/DeferredCompositor.hpp
├── license header (GPL, same as all headers)
├── #pragma once
├── Includes:
│   ├── Engine.hpp (engine access and Vulkan resources)
│   ├── GpuAttachmentBuffer.hpp (to read G-buffer images)
│   ├── vulkan/factory/GraphicsPipeline.hpp (pipeline creation helpers)
│   ├── scene/vk/FrameDataBinding.h*pp (scene descriptor binding pattern)
│   ├── scene/vk/LightDataBinding.hpp (light UBO binding access in shader)
│   ├── scene/vk/EnvironmentDataBinding.hpp (environment IBL binding access in shader)
│   ├── vulkan/factory/Sampler.hpp (sampler creation for G-buffer textures)
│   ├── uniforms/materials.h*pp (optional if we need to pass composite uniforms)
├── Public interface:
│   ├── DeferredCompositor(Engine* engine, std::shared_ptr<GpuAttachmentBuffer> gbuffer)
│   ├── ~DeferredCompositor() = default
│   
│   └── build(VkSampleCountFlagBits msaaCount) → creates pipeline
│       (gbuffer colors are already created at this point via GpuAttachmentBuffer.build())
│   
│   └── initFrameResources(DescriptorSetAllocator* allocator) → register descriptor requirements
│   
│   └── render(VkCommandBuffer cmd, uint32_t currentFrame,
│               const vulkan::Image* outputColorImage,  ← swapchain color image (single sample)
│               FrameResources& frameRes) → draws fullscreen quad for compositing
│   
│   └── setBrightness(float b), contrast(float c), exposure(float e) → push constant params
│   └── brightness(), contrast(), exposure() → getters
├── Protected/Private members:
│   ├── Engine* _engine
│   └── GpuAttachmentBuffer::pointer (_gbuffers)
│   ├── VkPipeline _pipeline → composite rendering pipeline (fullscreen quad + PBR lighting)
│   └── VkPipelineLayout _pipelineLayout → descriptor layouts
│       - set=0: G-buffer textures samplers (3 bindings for RGBA8 attachments)
│       - set=1: FrameDataBinding (scene uniform buffer = view + projection matrices)
│       - set=2: EnvironmentDataBinding (IBL samplers: irradiance, specular env, BRDF LUT)
│       - set=3: LightDataBinding (light UBO array, up to 8 lights)
│       - Push constant: {gamma: float, brightness: float, contrast: float, exposure: float}
│   ├── VK_DESCRIPTOR_SET_LAYOUT _gbufferDSLayout → layout for G-buffer bindings (set=0)
│   ├── VkSampler _gbufferSampler → sampler for sampling G-buffer textures (linear filter, clamp to edge)
│   └── std::vector<std::shared_ptr<vulkan::Image>> _gbufferResolveImages → pointer to resolved images
│       (obtained from GpuAttachmentBuffer resolve targets after blit)
```

### Design Decision: Descriptor Layout (5 sets like forward renderer)

The compositor has more bindings than the forward renderer because it includes additional G-buffer textures. To maintain compatibility with existing data-bindings, the pipeline layout will be:

| Set | Binding(s) | Content | How Obtained |
|-----|------------|---------|--------------|
| 0 | 0 — 2 | G-buffer texture samplers (albedo + normal + materials) | Created dynamically in `render()` via descriptor allocator (does not use data binding class because G-buffer is deferred-specific) |
| 1 | 0 | FrameUniforms (view + projection matrices) | Reuses existing `scene::vk::FrameDataBinding::newDescriptorSet()` |
| 2 | 0—3 | IBL samplers + EnvironmentUniforms UBO | Reuses existing `scene::vk::EnvironmentDataBinding` |
| 3 | 0 | Light uniform array (8 lights) | Reuses existing `scene::vk::LightDataBinding` |

> **Note: why G-buffers don't use `PipelineDataBinding`**
> Existing data-bindings (`FrameDataBinding`, `EnvironmentDataBinding`, etc.) create VBO buffers. G-buffers are textures; their layout is static but textures change per frame (swapchain resize). There are two options:
> 
> 1. Create a `GBufferDataBinding`: class following the abstract class pattern (like `PipelineDataBinding`) but for textures, handling VK descriptor image descriptors.
> 2. Bind G-buffers directly in the `render()` lambda (like `ColorAttachmentsCanvas` does inline).
> 
> **Decision: option 1** (create `GBufferDataBinding` as a utility class in the compositor phase). This maintains consistency with the rest of the codebase and allows reuse of the descriptor-pool registration pattern (`initFrameResources` → `requirePoolSizeRatio`).

---

## B) DeferredCompositor.cpp — Implementation

### Constructor
```cpp
DeferredCompositor::DeferredCompositor(Engine* engine, std::shared_ptr<GpuAttachmentBuffer> gbuffer)
    : _engine(engine), _gbuffers(gbuffer) {} // store pointer to G-buffer images
```

### build() — pipeline creation (3 shaders: vert + optional RT frag / non-RT frag)
1. Create `VkDescriptorSetLayout` for G-buffers: 3 bindings × COMBINED_IMAGE_SAMPLER.
2. Create `VkPipelineLayout`: add Ds layout 0 (G-buffers), get layouts from existing data bindings.
3. Add push constant range: `{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CompositePushConstants)}`.
4. Create shaders:
   - Vertex: `deferred_lighting.vert.spv` (fullscreen triangle strip quad).
5. Add optional fragment shader version:
   - If `engine->rayTracingSupported()` → add `"deferred_lighting_rt_shadows.frag.spv"`.
   - Else → add `"deferred_lighting.frag.spv"` (non-RT fallback).

**IMPORTANT NOTE:** The compositing only needs one shader — the RT vs non-RT variant is selected in `build()` conditionally on `engine->rayTracingSupported()`.

6. Pipeline config:
   - DepthTest disabled (no depth test in compositing).
   - Blending: disabled (write directly to output color image with alpha already accounted for).
   - Cull mode: `VK_CULL_MODE_FRONT_BIT` (back-face culling for full-quad since it renders a triangle strip back-facing).
   - Multisampling: disabled (single-sample, input G-buffers are already MSAA resolved).
   - Color attachment format: same as output swapchain image format.

See `ColorAttachmentsCanvas::build()` and `RendererBasicForward.cpp` for exact patterns.

### initFrameResources()
Register required pool sizes: G-buffer textures (3 COMBINED_IMAGE_SAMPLER) + scene/env/light uniform buffers.

### render() — compositing draw
1. Transition G-buffer resolve target images to `SHADER_READ_ONLY_OPTIMAL` (done via helper in GpuAttachmentBuffer).
2. Begin render with single-color attachment (swapchain color image), no depth testing.
3. Set viewport/scissor to outputImage extent (full-screen).
4. Create G-buffer descriptor set: bind all 3 resolved images as COMBINED_IMAGE_SAMPLER.
5. Create scene/env/light descriptor sets (reuse existing binding classes' `newDescriptorSet()`).
6. Set push constants: gamma, brightness, contrast, exposure.
7. `vkCmdBindPipeline` with composite pipeline.
8. Draw fullscreen quad: `vkDraw(cmd, 6, 1, 0, 0)` (same pattern as ColorAttachmentsCanvas).
9. `cmdEndRendering()`

---

## C) `shaders/src/deferred_lighting.vert.glsl`
### Full-screen triangle-strip vertex shader

```glsl
#version 450
#extension GL_ARB_shading_language_include: require
#include "lib/constants.glsl"

// Vertex shader outputs only UV (0..1) for full-screen quad.
layout(location = 0) in vec2 aPos;   // pre-built as [(-1, -1), (-1, 3), (3, -1)], etc.
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out vec2 vTexcoord;

void main()
{
    gl_Position = vec4(aPos, 0.0,1.0); // pass-through from input vertex
    vTexcoord = aTexCoord;              // [0, 1] quad UV range
}
```

No transform matrices needed because the compositing pass uses a fullscreen quad, not scene geometry. The vertices are already positioned as a full-screen triangle strip.

---

## D) `shaders/src/deferred_lighting.frag.glsl`
### Fragment shader for deferred lighting

This shader will read the G-buffers (albedo, tangent-space normals from view space) and calculate PBR lighting per pixel.

```glsl
#version 460  // Required for ray query (GL_EXT_ray_query); fallback to #version 450 if no RT
#extension GL_ARB_shading_language_include : require

// === G-buffer textures (set=0) ===
layout(set = 0, binding = 0) uniform sampler2D g_Albedo;     // albedo color
layout(set = 0, binding = 1) uniform sampler2D g_Normals;    // tangent-space normals
layout(set = 0, binding = 2) uniform sampler2D g_Materials;  // metalness(R), roughness(G), AO(B), emissive(A)

// === Scene data (set=1) ===
layout(set = 1, binding = 0) uniform SceneUniforms {
    mat4 viewMatrix;
    mat4 projectionMatrix;
} u_Scene;

// === Environment data (set=2) — IBL ===
layout(set = 2, binding = 0) uniform samplerCube u_Irradiance;
layout(set = 2, binding = 1) uniform samplerCube u_SpecularEnv;
layout(set = 2, binding = 2) uniform sampler2D u_BRDFLUT;
layout(set = 2, binding = 3) uniform EnvironmentUniforms {
    float maxReflectionLOD;
} u_Env;

// === Light data (set=3) ===
layout(set = 3, binding = 0) uniform LightData {
    Light lights[LIGHT_COUNT];
    uint lightCount;
} u_Lights;

// === Optional: ray tracing TLAS (set=4, only with #version 460 or extension) ===
#extension GL_EXT_ray_query : require
layout(set = 4, binding = 0) uniform accelerationStructureEXT u_TLAS;

// === Push constants: color correction params ===
layout(push_constant) uniform PC {
    float gamma;          // typically 2.2f, hardcoded
} pushConstant;

// === Outputs ===
layout(location = 0) out vec4 fragColor;

// === Input from vertex shader ===
layout(location = 0) in vec2 vTexcoord;

#include "lib/color_correction.glsl"
#include "lib/constants.glsl"
#include "lib/uniforms.glsl"
#include "lib/pbr.glsl"       // All PBR helper functions for Cook-Torrance
#include "lib/normal_map.glsl" // TBNMatrix helpers (optional: we have normal already in texture)
```

### Main function logic (pseudo-glsl flow):

```glsl
void main() {
    // 1. Sample G-buffers using texcoord (vTexcoord)
    vec4 albedo = texture(g_Albedo, vTexcoord);              // sRGB color input
    vec3 normalTS = texture(g_Normals, vTexcoord).xyz * 2.0 - 1.0; // unpack SNORM to [-1, 1]
    vec4 materialData = texture(g_Materials, vTexcoord);     // M=red, R=green, AO=blue, Emissive=pink
    float metalness = materialData.r;
    float roughness = max(materialData.g, 0.05);             // minimum roughness clamp
    float ao = materialData.b;
    vec3 emissive = materialData.a.rgb * materialData.a.a;   // Emissive (if any)

    // 2. Build camera parameters from scene uniform buffers
    vec2 ndc = vTexcoord * 2.0 - 1.0; // Convert UV to clip-space [-1, 1]
    float clipNear = -nearClip;      // Assume known near plane (constant or from uniforms)
    float clipFar = farClip;          // Assume known far plane
    
    // Unproject to get view space position
    vec4 clip = vec4(ndc, 0.0,1.0);   // Position on far plane (approximation)
    clip = inverse(u_Scene.projectionMatrix) * clip;
    vec4 viewPos = u_Scene.viewMatrix * clip;  // World position (actually: view space → invert
    vec3 fragWorld = inverse(u_Scene.viewMatrix) * viewPos; // This is world position approximation
    vec3 viewDir = normalize(fragWorld - cameraPosition); // Camera position from scene data (need to add)

    // Actually, we need the original PBRObjectData's model matrix and inverse view pos to recover world position at fragment level, but in deferred rendering we don't have per-object model matrices.
    
    // Better approach: store view-space position in G-buffer as well (replace normals with depth, or use a 3rd color buffer for position/depth).
    // 
    // Decision: Instead of reconstructing view space from depth, we add a G-buffer attachment for depth (or view-space position). This would be 4 color attachments + depth.
    // But wait, we already have a single-sample depth buffer from the depth prepass which can be used for reconstructing view position.
}
```

> **CRITICAL:** To calculate PBR lighting we need the fragment's position in view space or world space, because lighting functions (`calcRadiance`, etc.) need the distance to the light source.
> 
> In forward rendering this is calculated in the vertex shader (`inFragPos`). In deferred, we don't have access to that information per pixel.
> 
> **Options:**
> 1. **Reconstruct view-space position from depth + UV:** Add a single-sample (or MSAA) depth buffer to the G-buffer set. Then in compositor, reconstruct view-space position via: `viewPos = unproject(texcoord, depthValue)`.
> 2. **Store depth directly in G-buffer:** Use one of the existing color attachments to store view-space Z or position.
> 3. **Add a dedicated depth texture to the compositor** (read from the single-sample depth prepass buffer). This is the cleanest approach.
> 
> **Adopted decision:** Pass a 4th G-buffer texture to compositing that stores depth (or better, view-space position X,Y,Z as `R32G32B32_SFLOAT` or depth texture).
>
> **Simpler alternative:** Store a `VK_FORMAT_R32_SFLOAT` depth texture (single sample from prepass) as `buffer=3, binding=0` sampler in the compositor. This avoids needing a full view-position G-buffer since we can unproject using projection matrix inverse.

Actually, let me reconsider. Looking at the existing PBR shader (`pbr.glsl`), functions like `calcRadiance()` receive:
- View space normal (`N`): available from G-buffer normals.
- Frag position in world or view: This is needed to compute lighting contribution relative lights.
- View dir: Needed for `calcRadiance()`.

The cleanest approach is to **store the view-space position (XYZ as `R32G32B16_SFLOAT` or 4x float) in one of the G-buffer attachments**, replacing the emissive channel. But this significantly increases memory usage (from 128 bits/4 bytes per pixel = huge increase).

Given the existing infrastructure, the simplest approach is:

**Add a 4th G-buffer color attachment with format `VK_FORMAT_R32G32B16A16_SFLOAT` storing view-space position as (X, Y, nearZ, 1) where we can recover world space from inverse view matrix.** This would increase memory usage but is clean.

**Actually, the simplest approach that matches existing code:**
Use a separate depth texture sampler (from the single-sample depth prepass) and reconstruct position in the fragment shader. Here's a simpler approach:

**Add depth buffer as separate G-buffer accessor:**
In `DeferredCompositor`, read the depth value of the prepass from a shared sampler. In the shader: `float depth = texture(u_Depth, vTexcoord).r`, then unproject it to get view-space position using scene uniforms (viewMatrix, projectionMatrix). The existing `pbr.glsl` functions already receive a `vec3 fragWorldPos` — we need to compute it in the compositing shader.

For now, let's assume that the `DeferredCompositor` receives the depth buffer as 5th G-buffer texture (so `binding=3` in shader). Then in the compositor fragment:

```glsl
// Reconstruct view-space position from depth
float linearDepth = texture(u_DepthTex, vTexcoord).r;  // Linearized depth
vec4 clipPos = vec4(vTexcoord * 2.0 - 1.0, linearDepth, 1.0);
vec4 viewPos = inverse(u_Scene.projMatrix) * clipPos;
viewPos /= viewPos.w; // perspective divide, if we don't have it already because clipPos
vec3 worldPos = (u_Scene.viewMatrix * viewPos).xyz; // Actually, inverse for the view matrix
vec3 viewDir = normalize(worldPos - cameraPosition);   // Approximate (if we store the camera pos or compute it)
```

To make this simpler, add a `CameraData` uniform buffer (or extend SceneUniforms) that includes `worldPosition` and `viewMatrix`. Actually, the existing code already stores cameraWorldPos in a local variable (from `_scene->mainCamera()->ownerNode()->worldPosition()`). We can either:
- Extend `SceneData` to include `cameraWorldPos`.
- Pass it as a separate uniform buffer.

**Decision:** Add to `SceneUniforms::cameraWorldPos` as a vec3 (with padding for alignment). This is the least intrusive change.

---

## E) `shaders/src/deferred_lighting_rt_shadows.frag.glsl`

```glsl
#version 460
#extension GL_EXT_ray_query : require
#include "lib/color_correction.glsl"
#include "lib/constants.glsl"
#include "lib/uniforms.glsl"
#include "lib/pbr.glsl"

layout(set = 0, binding = 4) uniform accelerationStructureEXT u_TLAS; // Set=4 for TLAS
// All other layouts same as deferred_lighting.frag.glsl

void main() {
    // Same as above, but use ray-query shadow test inside lighting loop:
    
    for (uint i = 0; i < u_Lights.lightCount; i++) {
        // Shadow ray test:
        float tMax = 1000000.0; // or distance to light (for point lights)
        vec3 dir = normalize(lights[i].direction);  // ... or toLightDir = normalize(toTargetPos - fragPos)
        if (light type == LIGHT_TYPE_POINT) { 
            vec3 toLight = light[i].position - worldPos;
            tMax = length(toLight);
            dir = normalize(toLight);
        }
        
        // Trace ray through TLAS: 
        rayQueryEXT rq;
        rayQueryInitializeEXT(rq, u_TLAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT, 0xFF, worldPos + normal*0.01, 0, dir, tMax);
        while (rayQueryProceedEXT(rq)) { }
        
        if (rgtQueryGetResultTypeEXT(rg, true) == Gl_QueryCommittedIntersectionNoneEXT) {
            // light is visible — add contribution via loop:
            Lo += calcRadiance(...) // using same functions as pbr.glsl library
        }
    }
    
    vec3 color = ambient + Lo; // Same as forward renderer
    
    // Post-processing (same in both RT/non-RT):
    color = exposure(color, pushConstant.exposure);  // Reinhard tone mapping (or from color_correction.glsl)
    outColor = lineal2SRGB(vec4(color, 1.0), pushConstant.gamma);
    outColor = brightnessContrast(outColor, _brightness, _contrast); // push constants for color correction
}
```

---

## Relationship with `ColorAttachmentsCanvas`

| Aspect | ColorAttachmentsCanvas (existing) | DeferredCompositor (new) |
|--------|----------------------------------|--------------------------|
| Fullscreen quad rendering | Yes (`vkCmdDraw(cmd, 6, 1, 0, 0)`) | Yes (same technique with `aPos` attributes → full-screen triangle strip, vertex shader passthrough) |
| Texture samplers | G-buffer textures as image samplers | Same (samplers for 3/4 G-buffer textures) + scene/env/light binding |
| Pipeline creation | Fragment shader path passed as string in `build()` | Hardcoded `deferred_lighting.frag.spv` (RT or non-RT version depending on engine support) |
| Blending | Optional, disabled by default | Disabled (write directly to swapchain color image) |

---

## Phase 2 Checklist
- [ ] `DeferredCompositor::build()` creates pipeline with correct descriptor layouts (5 sets: G-buffers, scene, env, lights)
- [ ] `build()` selects RT vs non-RT fragment shader depending on engine ray-tracing support
- [ ] Full-screen vertex outputs correct position and UV (matching ColorAttachmentsCanvas pattern)
- [ ] Compositor fragment:
  - Reads all G-buffer textures
  - Samples depth prepass and reconstructs view-space position (or uses stored pos attribute)
- [ ] Lighting loop iterates lights array, calls PBR functions from lib/pbr.glsl
- [ ] Shadow test via ray-tracing `rayQueryEXT` when RT is supported, skips shadow otherwise
- [ ] Output: final color SRGB encoded with tone mapping + brightness/contrast push constants applied
