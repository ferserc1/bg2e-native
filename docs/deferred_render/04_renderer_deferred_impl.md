# Fase 3 — RendererDeferred (Implementación Completa)

## Objetivo

Implementar los métodos virtuales de `RendererDeferred` con la lógica completa del pipeline deferred:
1. **G-buffer pass:** renderiza geometría en los G-buffers (albedo, normales, materiales).
2. **Compositing pass:** renderiza un full-screen quad que lee los G-buffers y calcula iluminación PBR.
3. **Transparent passthrough:** renderiza objetos transparentes directamente al swapchain con alpha blending.

## Objetivo de la draw() per frame

```
renderer.draw(cmd, currentFrame, colorImage, depthImage, msaaDepthImage, frameResources)
│
├─ 1. Escena will-draw (actualizar scene graph)
├─ 2. Actualizar ray-tracing TLAS si es soportado (scene->rootNode → TLAS update)
├─ 3. Actualizar resources del ambiente (env.update, IBL updates)
│
├─ 4. G-BUFFER PASS:
│   ├─ 4a. Clear de todos los attachments (G-buffers + depth a black/0)
│   ├─ 4b. Begin rendering: multiple color attachments (G-buffers) + single-sample depth
│   ├─ 4c. Depth prepass (solo write depth, sin colores) → renderQueue.Opaque
│   ├─ 4d. G-buffer color pass (no write depth, color attachments) → renderQueue.Opaque
│   ├─ 4e. End rendering (G-buffer pass)
│   ├─ 4f. MSAA resolve de color attachments a single-sample targets (blit)
│   └─ 4g. Transicionar layouts: G-buffer MSAA → SHADER_READ_ONLY_OPTIMAL
│
├─ 5. COMPOSITING PASS:
│   ├─ 5a. Begin rendering: single color attachment (swapchain colorImage), sin depth
│   ├─ 5b. Set viewport/scissor a extencion del swapchain (full-screen quad)
│   ├─ 5c. Crear descriptor sets: scene, env, lights (reutilizar bindings existentes)
│   ├─ 5d. Crear descriptor set de G-buffers (muestreador con samplers)
│   ├─ 5e. Push constants (gamma, brightness, contrast, exposure)
│   ├─ 5f. Bind composite pipeline y render fullscreen quad
│   └─ 5g. End rendering (compositing pass)
│
├─ 6. TRANSPARENT PASS:
│   ├─ 6a. Begin rendering: color attachment del swapchain + depth testing (sin write)
│   ├─ 6b. Bind transparent pipeline con alpha blending
│   ├─ 6c. Crear descriptor sets por material: scene, object, env, lights
│   ├─ 6d. Renderizar renderQueue.Transparent + SolidTransparent directamente al colorImage
│   └─ 6e. End rendering (transparent pass)
│
├─ 7. Selección highlight si no es offscreen
│
└─ 8. Escena did-draw (resetar flags)
```

---

## Método `draw()` — Implementación paso a paso

### Pass 4: G-buffer rendering (depth prepass + color pass)

El G-buffer render se divide en **dos sub-pases**: primero depth-only, después color. Ambos usan el mismo pipeline (depth prepass para optimizar), pero la diferencia está en qué attachments se habilitan:

**Depth prepass:**
- Solo writes depth (single-sample). No color attachments. Esto optimiza el throughput porque no se escriben colores (albedo, normales, materiales).
- Todos lo objetos opacos escriben su profundidad.

**Color pass:**
- Depth test pero NO depth write (no-write). Solo color attachments.
- Los objetos opacos leen la profundidad del prepass para hacer depth testing, pero **no escriben**.

#### Pipeline G-buffer
Se crea en `build()` con:
- Vertex shader: reutilizar el existente `basic_forward.vert.spv` (ya TBT + uv outputs).
- Fragment shader: nuevo `deferred_gbuffer.frag.spv` (solo escribe G-buffers, NO calcula iluminación).
- **Depth write enabled** en prepass, disabled en color pass. En dynamic rendering se puede hacer esto con `enableDepthtest(true)` vs. `enableDepthtest(false)`.

```cpp
VkPipeline RendererDeferred::createGBufferPipeline(...) {
    bg2e::render::vulkan::factory::GraphicsPipeline plFactory(_engine);
    
    // Reutilizar vertex shader de forward renderer
    plFactory.addShader("basic_forward.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    
    // Fragment shader nuevo: solo escribe G-buffers
    plFactory.addShader("deferred_gbuffer.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    
    plFactory.setInputState<bg2e::render::vulkan::geo::Mesh>();
    
    // Depth format y prepass config: enable depth test + write for opaque objects.
    plFactory.setDepthFormat(_depthImageFormat);  // Single-sample depth format (D32_SFLOAT)
    plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);  // Depth write enabled for depth prepass
    
    plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    
    // MSAA: habilitar multisampling para color attachments → solo aplica si no-offscreen
    _isOffscreen ? plFactory.disableMultisample() : plFactory.enableMultisample();
    
    // Multi-color attachments: 3 G-buffer formats
    std::vector<VkFormat> gBufferFormats {
        VK_FORMAT_R8G8B8A8_UNORM,  // albedo
        VK_FORMAT_R8G8B8A8_SNORM,  // normal 
        VK_FORMAT_R8G8B8A8_UNORM   // materials (R=met, G=rough, B=ao, A=emissive)
    };
    plFactory.setColorAttachmentFormat(gBufferFormats); // Uses VkPipelineRenderingCreateInfo
    
    auto result = plFactory.build(_pipelineLayout);
    
    // Cleanup registration (engine->cleanupManager() push)
}
```

> **Nota sobre el G-buffer fragment shader:**
> Este shader debe escribir en 3 color attachments (locations 0, 1, 2). Usa los samplers de textura del material (como el forward renderer). No usa iluminación alguna. Escribe los datos directamente a G-buffers.
> 
> **Implementación GLSL del shader `deferred_gbuffer.frag.glsl`:**
```glsl
#version 450
#include "lib/constants.glsl"

layout(set = 1, binding = 0) uniform PBRObjectData {
    mat4 modelMatrix;
    materialParams;  // Material properties: albedo, normal map params, etc.
};

layout(set = 1, binding = 1) uniform sampler2D s_Albedo;
layout(set = 1, binding = 2) uniform sampler2D s_NormalMap;
// ... más texture samplers

layout(location = 0) out vec4 g_Albedo;
layout(location = 1) out vec4 g_NormalsTS;        // Tangent-space normals
layout(location = 2) out vec4 g_Materials;         // R=metalness, G=roughness, B=AO, A=(emissive)

// In: vertex shader outputs — normalTS (tangent space normals), uv0, etc.

vec4 SRGB2Lineal(vec4 srgbColor, float gamma) {
    if (srgbColor.a < 1e-6) return vec4(0.0);
    return pow(srgbColor.rgb / srgbColor.a, vec3(gamma)) * srgbColor.a;
}

void main() {
    // Sample albedo and convert from SRGB to linear
    vec4 rawAlbedo = texture(s_Albedo, inUV0 * materialParams.albedoScale);  // raw sRGB
    vec4 linearAlbedo = SRGB2Lineal(rawAlbedo, 2.2);
    g_Albedo = linearAlbedo; // Store in attachment (no SRGB encoding, will be displayed in compositor with LDR2SRGB)
    
    // Sample and convert normals to tangent space [0, 1] → [−1, 1]:
    vec3 normalTS = texture(s_NormalMap, inUV0 * materialParams.normalScale).xyz;
    g_NormalsTS = vec4(normalTS * 2.0 - 1.0, 1.0); // SNORM in [-1, +1]
    
    // Material properties: directly take from uniform or sampled texture
    g_Materials.r = materialParams.metallic;  // Metalness → R-channel
    g_Materials.g = materialParams.roughness;         // Roughness → G-channel
    g_Materials.b = materialParams.ao;                // AO → B-channel (if available)
    g_Materials.a = materialParams.emissive;          // Emissive → A-channel (if any)
}
```

#### Draw de geometry en G-buffer pass: depth prepass + color pass

```cpp
// === Depth prepass (no colors, only depth) ===
vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gBufferDepthPrepassPipeline);
_renderQueueVisitor.reset();   // Clear render queue for fresh pass? NO, keep it!
// Instead: just use the same _renderQueue. But we need to bind differently.

auto dsFunctionGBuffer = [&](...){ 
    // returns descriptor sets: { frameDS, objectDS, envDS, lightDS } (no RT needed for depth prepass!)
};

// Depth prepass: bind without color attachments
{
    // Begin rendering with only depth attachment (no color attachments = VK_ATTACHMENT_UNUSED for colors, use DynamicRendering)
    // In Vulkan 1.3 with dynamic rendering: pass vkPhysicalDeviceDynamicRendering::vkCmdBeginRendering with colorAttachmentCount=0
    // This is exactly what cmdClearImagesAndBeginRendering macro does - look at it's parameters.
}

// Actually, we need to be clever here: the existing `cmdClearImagesAndBeginRendering` macro takes a single color image and depth.
// For G-buffer pass we need 3 color attachments (no clear them to black) + depth.
// The macro might not support this, so we either:
// 1. Modify the macro to accept a vector<VkImageView> colorAttachments
// 2. Create new macros/helpers for multi-attachment rendering in vulkan/macros/all.hpp or graphics.hpp
// 3. Manually call vkCmdBeginRenderingKHR from VULKAN_CORE/Vulkan.hpp (but the macro is in macros/graphics.hpp or macros/all.hpp)

// Let's look at the `cmdClearImagesAndBeginRendering` macro:
```

**Veredict:** We need to add a new helper function/macro that supports multi-attachment begin render. In the existing `macros/graphics.hpp`, there is likely a macro for this or we need to create one.

This requires the `GraphicsPipeline` factory already supports multiple color attachment formats (it does via `setColorAttachmentFormat(vector<VkFormat>)` → VkPipelineRenderingCreateInfo with viewMask and colorAttachmentCount). The key is that the dynamic rendering create info should be constructed via a wrapper.

---

## Método `build()` — Setup inicial

```cpp
void RendererDeferred::build(Engine* engine, VkExtent2D initialExtent, ...) {
    _engine = engine;
    
    // Create G-buffer buffer (managed instance)
    _gBuffer = std::make_unique<GpuAttachmentBuffer>(engine, initialExtent);
    
    // Create the compositor (managed instance)
    _compositor = std::make_unique<DeferredCompositor>(engine, _gBuffer);
    
    // Create all bindings (reusing same pattern as RendererBasicForward)
    _frameDataBinding = std::make_unique<scene::vk::FrameDataBinding>(engine);
    _objectDataBinding = std::make_unique<scene::vk::ObjectDataBinding>(engine);
    _environmentDataBinding = std::make_unique<scene::vk::EnvironmentDataBinding>(engine);
    _lightDataBinding = std::make_unique<scene::vk::LightDataBinding>(engine);
    if (RT supported) _rtDataBinding = ... new RayTracingSceneDataBinding(engine);
    
    // Create environment resources for IBL (sky) — same as forward renderer
    _environment = std::make_unique<EnvironmentResources>(...);
    
    // Create pipelines:
    createGBufferPipelines(engine);  // depth prepass + color pass (both use same vert shader)
    createCompositorPipeline();  // Called from DeferredCompositor::build() with sampleCount
    
    if (!isOffscreen) {
        _selectionHighlight = new manipulation::SelectionHighlight(); ...;
    }
}
```

### Pipeline para G-buffer: depth prepass vs color pass

Para evitar tener dos pipelines (uno con depth-write y otro sin), el depth prepass puede reutilizar el mismo pipeline pero usar **different descriptor sets or same?** No, because the fragment shader for both passes is identical — depth prepass just writes nothing to color, only writes depth.

Wait: in the G-buffer fragment shader above (`deferred_gbuffer.frag.glsl`), we are writing to all 3 G-buffer attachments (locations 0, 1, 2). For the depth prepass we're writing **nothing**. This means:

```
Approach 1: Two different fragment shaders: one that writes G-buffers (color pass), one that only writes depth.
Approach 2: One fragment shader, use VK_ATTACHMENT_UNUSED for color attachments in the depth prepass render pass. In dynamic rendering this is done by passing `colorAttachmentCount: 0`. But we can't use the same pipeline — because it's bound with a specific render info (3 color attachments).
Approach 3: Use two different pipelines created from the same factory. The depth prepass pipeline is configured with `disableDepthtest()` no... wait, we need DEPTH WRITE for prepass. And depth format enabled:
```

Let me reconsider. For the depth prepass we want:
- Depth test + depth write enabled
- No color attachments (we skip writing to G-buffers entirely)

For the color pass we want:
- Depth test + depth write disabled (read-only depth)  
- 3 color attachments

These require **two different pipelines** because they are configured differently in VkPipelineRenderingCreateInfo. Two pipelines: `gBufferDepthPrepass` and `gBufferColor`.

---

## Método `initScene()` e `initFrameResources()`

**Pattern to copy exactly from `RendererBasicForward`:**
- `initScene()`: calls `_scene->setSceneRoot(sceneRoot)`, creates sky dome texture generator, environment resource build (`_environment.build(...)`), calls `_scene->updateLights()`, updates resize viewport visitor.
- `initFrameResources()`: calls `_frameDataBinding->...`, ... etc., then `_environment->....`

---

## Método `update()` — actualización por frame

```cpp
void RendererDeferred::update(float delta) {
    _scene->willUpdate();
    
    // Update scene visitors (same pattern from forward renderer)
    _updateVisitor.update(_scene->rootNode, delta);

    // Update environment (IBL) if changed
    if (_scene->mainEnvironment() && ...) { ...swap env... }
    
    // Update light resources (same pattern from forward renderer)
    auto lightComponents = _scene->lightComponents();
    ...loop through lights...

    _scene->didUpdate();
}
```

---

## Método `resize()` — redimensionar recursos por swapchain resize

```cpp
void RendererDeferred::resize(VkExtent2D newExtent) {
    _viewportExtent = newExtent;
    _scene->willResize();
    
    // Resize G-buffer textures (they are resized according to new extent)
    _gBuffer->resize(newExtent); // or call build() with new extent
    
    _scene->didResize();
}
```

---

## Método `cleanup()` — destrucción limpia

```cpp
void RendererDeferred::cleanup() {
    // Cleanup order: reverse of creation order
    _renderQueue.cleanup();  // clear render queue
    
    // Cleanup compositor
    _compositor->cleanup();
    
    // Cleanup G-buffer
    _gBuffer->cleanup();  // destroys all Vulkan images
    
    // Cleanup scene (managed by the engine cleanup manager)
    _engine->cleanupManager().push([&](VkDevice) { _scene.reset(); });
    
    // Cleanup environment (IBL)
    _environment->cleanup();  // if not already handled
    
    // Cleanup selection highlight (if any)
}
```

---

## Método `createPipelines()` — creación de pipelines

El RendererDeferred tiene **5 pipelines**:
1. `gBufferDepthPrepassPipeline` → depth prepass (depth-write, no color attachments)
2. `gBufferColorPipeline` → G-buffer color pass (no depth-write, 3 color attachments)
3. `compositorPipeline` → compositing pass (fullscreen quad, no depth testing)
4. `forwardOpaquePipeline` → transparent passthrough: opaque objects render directly to swapchain
5. `forwardTransparentPipeline` + `solidTransparentPipeline` → same as above with alpha blending

Wait, for the transparent pass we can reuse existing forward rendering patterns. But actually in deferred rendering, there is no point in writing transparent objects to G-buffers (they need alpha blending which defeats the purpose of G-buffer material attributes). So transparent objects render **directly to swapchain**.

```cpp
void RendererDeferred::createCompositorPipeline(Engine* engine) {
    // Delegate to compositor class's build method:
    _compositor->build(_isOffscreen ? VK_SAMPLE_COUNT_1_BIT : _sampleCount);
}

void RendererDeferred::createTransparentPipelines(Engine* engine, VkPipelineLayout layout) {
    // Same pipelines as forward renderer but without G-buffer writes — write directly to swapchain color image
    // This can reuse basic_forward.vert.spv and deferred_composite.frag.spv (or just skip lighting for transparency)
    
    // Actually, transparent objects don't need lighting computation in deferred mode; they just render directly.
    // Use basic_forward.frag.glsl for proper transparency handling, just like forward renderer
}
```

---

## Render queue visitor y data bindings en el G-buffer pass

### Crear render queue (dentro de draw())
```cpp
// Reset and populate the render queue from scene objects:
_renderQueueVisitor.enqueue(_scene->rootNode(), &_renderQueue);
```

### Función de descriptor sets para G-buffer color pass
Similar a `RendererBasicForward::dsFunction` lambda in draw():

```cpp
auto dsFunctionForGBuffer = [&](MaterialBase* mat, const glm::mat4& transform) {
    auto objectDS = _objectDataBinding->newDescriptorSet(
        frameResources, mat, transform);
    return vectorVkDescriptorSets{sceneDS, objectDS, envDS, lightDS};  // No RT needed for depth prepass!
    // Actually we may need it for transparent pass, but skip for G-buffer depth+color
};
```

### Render draw del render queue a G-buffers (doble sub-pase, same pipeline):

```cpp
vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gBufferColorPipeline);

_renderQueue.render(
    RenderQueueType::Opaque, 
    cmd, _pipelineLayout, dsFunctionForGBuffer, cameraWorldPos
);

// Then for transparent objects (no G-buffer write), we skip them in the G-buffer pass
```

---

## Checklist de la fase 3

- [ ] `draw()` implementa los 3 pases (G-buffer → composite → transparent)
- [ ] Pases G-buffer: depth prepass (sin color attachments) + color pass (3 color attachments, sin write depth)
- [ ] Pases compositing: fullscreen quad reading G-buffers, calcula iluminación PBR
- [ ] Pases transparentes: objetos transparent se renderizan directamente al swapchain
- [ ] `build()` crea 2 G-buffer pipelines (depth prepass + color + compositor)
- [ ] `createPipelines` crea todos los pipelines correctos (seleccionando RT vs no-RT)
- [ ] El `draw()` llama a `gBuffer->resolve(cmd)` para hacer blit de MSAA antes del compositing pass
- [ ] Todas las transiciones de layout Vulkan son correctas

---

## Notas importantes sobre integración con el pipeline existente

### Reutilización de shaders vs nuevos shaders

| Shader | Uso en renderer forward | Uso en deferred renderer |
|--------|------------------------|--------------------------|
| `basic_forward.vert.spv` | OPA: vertex shader for opaque and transparent objects in forward pass | **G-buffer color/depth prepass:** vertex shader para geometría (TBF) |
| `basic_forward.frag.spv` / `_rt_shadows.` | OPA fragment shader for forward PBR lighting calculation  | No se reutiliza |
| `deferred_gbuffer.frag.spv` (nuevo) | —  | G-buffer fragment shader, solo escribe los G-buffers albedo, normal, materials; sin iluminación |
| `deferred_lighting.vert.spv` (nuevo) | — | Compositing: fullscreen quad vertex passthrough |
| `deferred_lighting.frag.spv` (nuevo) | —  | Compositing: fragment shader, lectura de G-buffers + iluminación PBR (con/sin RT) |

### Pipeline layout comparison: forward vs deferred

| Set | Forward (set=0..4) | Deferred compositor (set=0..3+1 RT) |
|-----|-------------------|-------------------------------------|
| Set 0 | Scene (FrameData) | G-buffer textures (3 samplers: albedo, normals, materials) |
| Set 1 | Object data binding (model + material uniforms + 5 text samplers) | Scene Data (view/projection) — same binding as forward-set=0 |
| Set 2 | Environment data (IBL+samplers) | Env/IBL Data — same binding as forward-set=2
| Set 3 | Light data (light buffer array) same as forward-set=3
| Set 4 | RT scene TLAS (if supported) — optional. Same as forward-set=4 if RT
