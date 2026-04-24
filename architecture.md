## Tabla de Contenidos

1. [Arquitectura Actual](#1-arquitectura-actual)
2. [Diagnóstico de Dependencias Vulkan](#2-diagnóstico-de-dependencias-vulkan)
3. [Arquitectura Propuesta](#3-arquitectura-propuesta)
4. [Plan de Implementación por Fases](#4-plan-de-implementación-por-fases)
5. [Decisiones de Diseño](#5-decisiones-de-diseño)

---

## 1. Arquitectura Actual

### 1.1 Estructura del Proyecto

```
bg2e-native/
├── lib/                          # Motor gráfico compartido (libbg2e)
│   ├── include/bg2e/            # Headers públicos
│   │   ├── all.hpp              # Include maestro
│   │   ├── common.hpp           # Defines de plataforma, macros BG2E_API
│   │   ├── math/                # Capa 0: proyecciones, utilidades (GLM)
│   │   ├── base/                # Capa 0: Color, Camera, Texture, MaterialAttributes
│   │   ├── json/                # Capa 1: parser JSON para serialización
│   │   ├── geo/                 # Capa 1: geometría procedural (Mesh, primitives)
│   │   ├── render/              # Capa 2: Engine, Renderer, RenderLoop, Textures
│   │   │   ├── vulkan/          # Backend Vulkan específico
│   │   │   │   ├── factory/     # Builders: pipelines, shaders, samplers
│   │   │   │   ├── geo/         # Wrappers de mallas Vulkan
│   │   │   │   ├── rt/          # Ray tracing meshes (BLAS/TLAS)
│   │   │   │   └── macros/      # Helpers C++ para Vulkan
│   │   │   └── uniforms/        # Structs PBR para shaders
│   │   ├── scene/               # Capa 3: Node, Component, Drawable, Visitors
│   │   │   └── vk/              # Bindings de descriptor sets (Vulkan)
│   │   ├── manipulation/        # Capa 5: selección, highlight, pick visitor
│   │   ├── utils/               # Capa 6: TextureCache, MaterialSerializer
│   │   ├── db/                  # Capa 4: glTF, OBJ, .bg2 format loading
│   │   ├── app/                 # Capa 7: Application, MainLoop, Input
│   │   └── ui/                  # Capa 8: UserInterface, Windows, Widgets
├── lib/src/bg2e/                # Implementaciones .cpp
├── apps/model_edit/             # Editor de modelos (app principal)
├── examples/{01_setup..14_bg2_model_load}/  # 15 ejemplos progresivos
├── shaders/src/                 # Shaders GLSL (.glsl → .spv)
│   └── lib/                     # Librería compartida GLSL (PBR functions)
├── assets/                      # Texturas, HDR envmaps, modelos test
└── cmake/                       # Utils CMake (compile_shaders, bundle_app)
```

### 1.2 Capas del Motor (0-8)

| Capa | Módulo | Dependencias Internas | Propósito |
|------|--------|----------------------|-----------|
| 0 | `math`, `base` | Ninguna (foundation) | Matemáticas, colores, texturas CPU, cámaras |
| 1 | `json`, `geo` | Capa 0 | JSON, geometría procedural (Mesh, primitives) |
| 2 | `render` | Capas 0-1 | Engine, Renderer, RenderLoop, Texturas GPU |
| 3 | `scene` | Capas 0-2 | Scene graph (Node + Component), visitors |
| 4 | `db` | Capas 0-3 | Carga de assets (glTF, OBJ, .bg2) |
| 5 | `manipulation` | Capas 0-3 | Selección, highlight, ray picking |
| 6 | `utils` | Capas 0-3 | TextureCache, MaterialSerializer |
| 7 | `app` | Capas 0-8 | Application, MainLoop, InputManager |
| 8 | `ui` | Capas 0-7 | ImGui wrapper, widgets, editores |

### 1.3 Shader System

**Compilación:** GLSL en `shaders/src/` se compila a `.spv` (SPIR-V) via `glslang` del Vulkan SDK en build time.

**Shaders de producción (16 archivos):**
- `basic_forward.vert.glsl` / `.frag.glsl` — PBR forward rendering principal
- `skybox_renderer.*.glsl` — Skybox rendering
- `cubemap_renderer.vert.glsl` — Cubemap genérico (IBL)
- `sphere_to_cubemap.*.glsl` — Equirectangular → cubemap
- `irradiance_map_renderer.frag.glsl` — Irradiance map (diffuse IBL)
- `specular_reflection_renderer.frag.glsl` — Prefiltered env map (specular IBL)
- `brdf_lut.comp.glsl` — BRDF LUT (compute shader, 16x16)
- `color_correction.comp.glsl` — Post-processing sRGB (compute, GLSL 460)
- `pick_selection.*.glsl` — Picking/selection (object ID encode)
- `selection_highlight.*.glsl` — Selection outline effect
- `color_attachment_canvas.vert.glsl` — Full-screen quad para multi-attachment

**Librería compartida GLSL (`shaders/src/lib/`):**
- `constants.glsl` — Definiciones PI, etc.
- `color_correction.glsl` — lineal2SRGB, SRGB2Lineal, brightnessContrast, exposure
- `normal_map.glsl` — TBNMatrix para normal mapping
- `uniforms.glsl` — Structs PBRMaterialData, Light; funciones de sampling
- `pbr.glsl` — Cook-Torrance BRDF: fresnelSchlick, distributionGGX, geometrySmith, calcRadiance, calcAmbientLight

**Descriptor Set Layout (4 sets):**

| Set | Binding | Tipo Vulkan | Contenido |
|-----|---------|-------------|-----------|
| 0 | 0 | `UNIFORM_BUFFER` | SceneData: view + projection matrices (128 bytes) |
| 1 | 0 | `UNIFORM_BUFFER` | PBRObjectData: model matrix + material (256+ bytes) |
| 1 | 1-5 | `COMBINED_IMAGE_SAMPLER` | Albedo, normal, metalness, roughness, AO textures |
| 2 | 0-1 | `COMBINED_IMAGE_SAMPLER` | Irradiance map + prefiltered env map (cubemaps) |
| 2 | 2 | `COMBINED_IMAGE_SAMPLER` | BRDF LUT (2D texture, 256x256 rgba16f) |
| 2 | 3 | `UNIFORM_BUFFER` | EnvironmentData: maxReflectionLOD (4 bytes) |
| 3 | 0 | `UNIFORM_BUFFER` | LightData: hasta 8 lights (416 bytes) |

**Push Constants:** gamma, brightness, contrast, exposure

### 1.4 Pipeline de Renderizado (Forward PBR)

```
RenderLoop::run()
  └── per-frame:
        beginFrame() → acquireNextImage() → waitForFence()
          └── RenderLoopDelegate::render(cmdBuffer)
                └── RendererBasicForward::draw(cmd, frameIndex, colorImage)
                      └── DrawVisitor visita scene graph
                            └── DrawableComponent::draw(transform, cmd, layout, cb)
                                  └── ObjectDataBinding::newDescriptorSet(material, transform)
                                  └── renderMesh->drawSubmesh(cmd, layout, descriptorSets, submesh)
                      └── vkCmdPushConstants(gamma, brightness, contrast, exposure)
                      └── vkCmdBindPipeline() + vkCmdDrawIndexed() por submesh
          └── UserInterface::draw(cmd, imageView)  (ImGui rendering)
          └── vkQueueSubmit() → queuePresent()
        endFrame() → signalFence()
```

---

## 2. Diagnóstico de Dependencias Vulkan

### 2.1 Mapa de Contaminación por Capa

| Capa | Nivel | Vulkan en headers públicos? | Vulkan en .cpp solo? | Severidad |
|------|-------|----------------------------|---------------------|-----------|
| `render/` | 2 | **SÍ** — casi todos los headers | **SÍ** — todas las implementaciones | CRÍTICA |
| `scene/` | 3 | **SÍ** — Drawable, DrawVisitor, Mesh | **SÍ** — vk/bindings, RenderQueueVisitor | CRÍTICA |
| `db/` | 4 | **SÍ** — mesh_bg2, mesh_obj, scene_gltf | NO (solo .hpp) | ALTA |
| `app/` | 7 | **SÍ** — Application, MainLoop | NO (solo .hpp) | MEDIA |
| `ui/` | 8 | **SÍ** — UserInterface, TextureWidgets | NO (solo .hpp) | ALTA |
| `math/base` | 0-1 | NO | NO | OK |

### 2.2 Vulkan Types en Headers Públicos (por módulo)

#### `render/Engine.hpp` — CRÍTICO
```cpp
// Tipos Vulkan en firmas públicas:
const vulkan::Instance& instance() const;
const vulkan::PhysicalDevice& physicalDevice() const;
const vulkan::Surface& surface() const;
const vulkan::Device& device() const;
vulkan::Swapchain& swapchain();
vulkan::Command& command();
vulkan::DescriptorSetAllocator& descriptorSetAllocator();
vulkan::FrameResources& currentFrameResources();
VmaAllocator allocator() const;                                    // Raw VMA handle
void destroyBuffer(VkBuffer buffer, VmaAllocation allocation);     // Raw Vulkan types
```

#### `render/Renderer.hpp` — CRÍTICO
```cpp
#include <vulkan/vulkan.h>  // Include directo de vulkan.h

virtual void resize(VkExtent2D newExtent) = 0;
virtual void draw(VkCommandBuffer cmd, uint32_t currentFrame, 
                  const vulkan::Image* colorImage,
                  const vulkan::Image* depthImage) = 0;
```

#### `render/Texture.hpp` — ALTO
```cpp
VkSampler _sampler = VK_NULL_HANDLE;  // Miembro con tipo Vulkan crudo
```

#### `render/RenderQueue.hpp` — ALTO
```cpp
struct Item {
    std::shared_ptr<render::vulkan::geo::Mesh> renderMesh;  // Vulkan mesh
};
void enqueue(std::shared_ptr<render::vulkan::geo::Mesh>, ...);
void render(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, ...);
```

#### `render/RendererBasicForward.hpp` — ALTO
```cpp
VkPipeline getPipeline() const;
VkPipelineLayout getPipelineLayout() const;
virtual void resize(VkExtent2D newExtent) override;
```

#### `render/CubemapRenderer.hpp` — ALTO
```cpp
VkSampler _skyImageSampler;
VkDescriptorSetLayout _descriptorSetLayout;
VkPipelineLayout _layout;
VkPipeline _pipeline;
```

#### `render/SkyboxRenderer.hpp` — ALTO
```cpp
VkDescriptorSetLayout _dsLayout;
VkPipelineLayout _pipelineLayout;
VkPipeline _pipeline;
```

#### `render/SphereToCubemapRenderer.hpp` — ALTO
```cpp
VkPipelineLayout _pipelineLayout;
VkPipeline _pipeline;
VkDescriptorSetLayout _dsLayout;
std::unique_ptr<vulkan::Buffer> _projectionDataBuffer;
```

#### `render/ColorAttachmentsCanvas.hpp` — ALTO
```cpp
VkPipeline _pipeline;
VkPipelineLayout _pipelineLayout;
VkDescriptorSetLayout _attachmentsDSLayout;
VkSampler _attSampler;
```

#### `render/RenderLoop.hpp` — ALTO
```cpp
void render(VkCommandBuffer cmd, const vulkan::Image* colorImage);
```

#### `render/RenderLoopDelegate.hpp` — ALTO
```cpp
virtual VkImageLayout render(VkCommandBuffer cmd, ...) = 0;
virtual void swapchainResized(VkExtent2D) = 0;
```

#### `render/GPUProcess.hpp` — ALTO
```cpp
void addBinding(vulkan::Image*, VkDescriptorType, VkImageLayout);
void addBinding(vulkan::Buffer*, VkDescriptorType);
```

#### `render/GPUTextureGenerator.hpp` — ALTO
```cpp
virtual vulkan::Image* createImage(VkFormat, VkExtent2D, VkImageUsageFlags) = 0;
```

#### `scene/Drawable.hpp` — CRÍTICO
```cpp
#include <bg2e/render/vulkan/geo/Mesh.hpp>
#include <bg2e/render/vulkan/rt/RayTracingMesh.hpp>

typedef std::function<std::vector<VkDescriptorSet>(
    MaterialBase* material, const glm::mat4& transform, uint32_t submeshIndex
)> DrawFunction;

virtual void draw(VkCommandBuffer cmd, VkPipelineLayout layout, 
                  DrawFunction cb, VkPipelineBindPoint bp) = 0;
```

#### `scene/Mesh.hpp` — ALTO
```cpp
#include <bg2e/render/vulkan/geo/Mesh.hpp>
using RenderMesh = render::vulkan::geo::MeshPNUUT;  // Typedef a Vulkan type
```

#### `scene/DrawVisitor.hpp` — ALTO
```cpp
VkCommandBuffer _commandBuffer;
VkPipelineLayout _pipelineLayout;
```

#### `scene/DrawableComponent.hpp` — ALTO
```cpp
virtual void draw(const glm::mat4& nodeTransform, VkCommandBuffer cmd, 
                  VkPipelineLayout layout, DrawFunction cb,
                  VkPipelineBindPoint bp) = 0;
```

#### `scene/ResizeViewportVisitor.hpp` — MEDIO
```cpp
void visit(VkExtent2D extent);  // Vulkan type in signature
```

#### `scene/vk/*.hpp` (6 archivos) — CRÍTICO
Todos incluyen `<bg2e/render/vulkan/FrameResources.hpp>` y usan `VkDescriptorSetLayout`, `VkDescriptorSet`.

#### `ui/UserInterface.hpp` — ALTO
```cpp
#include <bg2e/render/Engine.hpp>
#include <SDL2/SDL.h>

VkFence _uiFence;
VkCommandBuffer _commandBuffer;
VkCommandPool _commandPool;
VkDescriptorPool _imguiPool;

void draw(VkCommandBuffer cmd, VkImageView targetImageView);
```

#### `ui/TextureWidgets.hpp` — MEDIO
```cpp
#include <bg2e/render/Texture.hpp>
VkDescriptorSet _textureDS = VK_NULL_HANDLE;
```

#### `db/mesh_bg2.hpp` — ALTO
```cpp
#include <bg2e/render/Engine.hpp>
Bg2Mesh* loadDrawableBg2(const std::filesystem::path&, render::Engine*);
```

#### `db/mesh_obj.hpp` — ALTO
```cpp
#include <bg2e/render/Engine.hpp>
std::shared_ptr<scene::Drawable> loadDrawableObj(const std::filesystem::path&, render::Engine*);
```

#### `db/scene_gltf.hpp` — ALTO
```cpp
#include <bg2e/render/Engine.hpp>
scene::Node* loadGltf(const std::filesystem::path&, render::Engine*);
```

#### `app/MainLoop.hpp` — MEDIO
```cpp
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/RenderLoop.hpp>

render::Engine _engine;
render::RenderLoop _renderLoop;
```

#### `app/GPUSelectionDialog.hpp` — MEDIO
```cpp
#include <bg2e/render/vulkan/PhysicalDevice.hpp>  // Direct vulkan include
```

### 2.3 Vulkan API Calls Directas Fuera de `render/vulkan/`

| Archivo | Llamaradas Vulkan Directas |
|---------|---------------------------|
| `render/Texture_render.cpp` | `vkCreateSampler()`, `vkDestroySampler()` |
| `render/RendererBasicForward.cpp` | `vkCmdPushConstants()`, `vkCmdBindPipeline()`, `vkDestroyPipelineLayout()`, `vkDestroyDescriptorSetLayout()` |
| `render/CubemapRenderer.cpp` | `vkCreateImageView()`, `vkDestroyImageView()`, `vkDestroySampler()`, `vkCmdClearColorImage()`, `vkCmdBindPipeline()`, `vkCmdPushConstants()`, `vkCmdBindDescriptorSets()`, `vkCreatePipelineLayout()`, `vkDestroyPipelineLayout()`, `vkDestroyDescriptorSetLayout()`, `vkDestroyPipeline()` |
| `render/SkyboxRenderer.cpp` | `vkCmdBindPipeline()`, `vkCmdBindDescriptorSets()`, `vkCmdPushConstants()`, `vkDestroyPipeline()`, `vkDestroyPipelineLayout()`, `vkDestroyDescriptorSetLayout()` |
| `render/SphereToCubemapRenderer.cpp` | `vkCmdBindPipeline()`, `vkCmdPushConstants()`, `vkCmdBindDescriptorSets()`, `vkDestroyPipeline()`, `vkDestroyPipelineLayout()`, `vkCreateImageView()`, `vkDestroyImageView()`, `vkDestroyDescriptorSetLayout()` |
| `render/ColorAttachmentsCanvas.cpp` | `vkCmdBindPipeline()`, `vkCmdDraw()`, `vkCmdBindDescriptorSets()`, `vkDestroyPipeline()`, `vkDestroyPipelineLayout()`, `vkDestroyDescriptorSetLayout()`, `vkDestroySampler()` |
| `render/GPUProcess.cpp` | `vkCmdBindPipeline()`, `vkCmdBindDescriptorSets()`, `vkCmdDispatch()`, `vkDestroyPipeline()`, `vkDestroyPipelineLayout()`, `vkDestroyDescriptorSetLayout()` |
| `render/RenderLoop.cpp` | `vkWaitForFences()`, `vkResetFences()`, `vkBeginCommandBuffer()`, `vkCmdResolveImage()`, `vkEndCommandBuffer()`, `queueSubmit2()`, `queuePresent()` |
| `render/EnvironmentResources.cpp` | `vkDestroySampler()` (cleanup lambdas) |

### 2.4 Acoplamientos Críticos Identificados

#### A. `render::Engine` es un wrapper de Vulkan puro
- Todos los accessors devuelven tipos `vulkan::` (Instance, PhysicalDevice, Surface, Device, Swapchain, Command)
- Miembro: `VmaAllocator _allocator` (raw VMA handle)
- Método: `destroyBuffer(VkBuffer, VmaAllocation)` con tipos Vulkan crudos
- **No es una abstracción gráfica, es un wrapper de Vulkan**

#### B. `render::Renderer` (clase abstracta) es una trampa
- Parece abstracción pero sus métodos virtuales toman `VkCommandBuffer`, `VkExtent2D`, `vulkan::Image*`
- Cualquier implementación debe conocer Vulkan internamente

#### C. Scene layer depende directamente de `render/vulkan/geo::Mesh`
- `scene/Mesh.hpp`: typedef `RenderMesh = render::vulkan::geo::MeshPNUUT`
- `scene/Drawable.hpp`: incluye `<bg2e/render/vulkan/geo/Mesh.hpp>` y `<bg2e/render/vulkan/rt/RayTracingMesh.hpp>`
- `DrawableBase::DrawFunction` devuelve `std::vector<VkDescriptorSet>`

#### D. `scene/vk/` bindings están en la capa equivocada
- 6 archivos de bindings viven en `scene/vk/` pero son puramente Vulkan
- Son incluidos por `scene/all.hpp`, haciendo que incluir scene = incluir vulkan

#### E. UI layer tiene Vulkan directo
- `UserInterface` almacena: `VkFence`, `VkCommandBuffer`, `VkCommandPool`, `VkDescriptorPool`
- Método `draw(VkCommandBuffer, VkImageView)` toma handles Vulkan crudos

#### F. DB layer pasa `Engine*` para upload a GPU
- `loadDrawableObj(path, Engine*)`, `loadDrawableBg2(path, Engine*)`, `loadGltf(path, Engine*)`
- El upload a GPU (VMA allocation + buffer creation) ocurre dentro de `Drawable::load(engine)`

### 2.5 Gráfico de Dependencias Crítico

```
render/Engine.hpp
  ├── render/vulkan/Instance.hpp
  ├── render/vulkan/common.hpp          (vma/vk_mem_alloc.h, vulkan/vulkan.h)
  ├── render/vulkan/Command.hpp
  ├── render/vulkan/Swapchain.hpp
  ├── render/vulkan/extensions.hpp
  ├── render/vulkan/CleanupManager.hpp
  ├── render/vulkan/FrameResources.hpp
  ├── render/vulkan/Surface.hpp
  ├── render/vulkan/PhysicalDevice.hpp
  └── render/vulkan/Device.hpp

render/Renderer.hpp
  ├── #include <vulkan/vulkan.h>        ← DIRECT INCLUDE
  └── render/vulkan/Image.hpp

render/RendererBasicForward.hpp
  ├── scene/vk/FrameDataBinding.hpp     ← SCENE MODULE DEPENDS ON VULKAN
  ├── scene/vk/ObjectDataBinding.hpp
  ├── scene/vk/EnvironmentDataBinding.hpp
  └── scene/vk/LightDataBinding.hpp

scene/all.hpp
  ├── scene/vk/all.hpp                  ← VULKAN INCLUSO EN SCENE
  └── scene/Drawable.hpp                ← includes render/vulkan/geo/Mesh

db/mesh_obj.hpp, mesh_bg2.hpp, scene_gltf.hpp
  └── render/Engine.hpp                 ← DB layer depends on Vulkan engine

ui/UserInterface.hpp
  ├── render/Engine.hpp
  └── SDL2/SDL.h                        ← Vulkan members: VkFence, VkCommandBuffer...

app/MainLoop.hpp
  ├── render/Engine _engine             ← Owns Vulkan engine directly
  └── render/RenderLoop _renderLoop
```
