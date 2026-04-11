# Project Skills

## 1. Project overview

bg2 engine es un motor gráfico en C++ orientado a aplicaciones profesionales que requiere integración nativa con el sistema operativo y escenas cargadas/almacenadas en tiempo real. Utiliza Vulkan como backend de renderizado con soporte para PBR (Physically Based Rendering), skyboxes, envmap irradiance/specular, y múltiples color attachments. El motor proporciona un sistema de escena jerárquico basado en Node/Component, UI con Dear ImGui, y soporte para escenas glTF y formato binario propio (.bg2). Diseñado para ser ligero y modular, con código dividido en núcleo (lib/), ejemplos (examples/) y aplicaciones (apps/).

## 2. Directory structure

```
lib/include/bg2e/                # Headers principales del motor
  render/                       # Renderizado Vulkan y abstracto
    Engine.hpp                  # Core Vulkan: Instance, Device, Swapchain, FrameResources
    Renderer.hpp                # Interface abstracta de renderizado
    RendererBasicForward.hpp    # Renderizador forward PBR implementado
    ColorAttachments.hpp        # Múltiples render targets (G-buffers)
  scene/                        # Sistema de escena
    Scene.hpp                   # Contenedor raíz de la escena
    Node.hpp                    # Nodos del árbol de escena con components
    Component.hpp               # Base para todos los componentes
    Visitor pattern classes     # UpdateVisitor, DrawVisitor, InputVisitor
  ui/                           # Interface de usuario (Dear ImGui wrapper)
    UserInterface.hpp           # UI wrapper con delegates
  base/                         # Tipos básicos: Color, Camera, MaterialAttributes, Texture
  db/                           # Base de datos de carga/guardado: glTF, bg2, OBJ
  json/                         # Parser JSON para serialización de escenas
  geo/                          # Geometría procedural: sphere, cube, plane, cylinder
  app/                          # Application lifecycle y main loop
  math/                         # Proyecciones, utilidades matemáticas (GLM wrapper)

lib/src/bg2e/                   # Implementación de todos los módulos
  render/vulkan/                # Wrapper Vulkan de bajo nivel
    Instance.hpp, Device.hpp   # Creación de VkInstance y VkDevice
    Swapchain.hpp              # Gestión de swapchain con MSAA
    FrameResources.hpp         # Data por-frame: command buffers, semaphores
    DescriptorSetAllocator.hpp # Pool de descriptor sets reutilizables
    factory/                   # Builder pattern para pipelines y recursos
      ShaderModule.hpp         # Carga de shaders .spv
      GraphicsPipeline.hpp     # Builder de graphics pipelines
      PipelineLayout.hpp       # Configuración de pipeline layout
    CleanupManager.hpp         # Gestión automática de recursos

shaders/                        # Shaders GLSL organizados por funcionalidad
  src/basic_forward.*          # Pipeline PBR forward con iluminación múltiple
  src/skybox_renderer.*        # Renderizado de skybox con envmap
  src/lib/                     # Shaders shared: pbr.glsl, uniforms.glsl, normal_map.glsl
    pbr.glsl                   # Cook-Torrance BRDF con GGX, geometry Smith
    uniforms.glsl              # Structs de material y luz
  src/lib/pbr.glsl             # Funciones PBR: fresnelSchlick, distributionGGX

examples/                       # Ejemplos de uso del motor
  01_setup/                    # Setup básico Vulkan con render loop
  02_ui/                       # UI con Dear ImGui integrada
  13_pbr_demo/                 # Demo completa PBR: materiales, lights, envmap
  10_scene_objects/            # Escena compleja con múltiples objetos y data bindings
  debug-app/                   # Aplicación de debug con carga glTF

apps/model_edit/                # Editor de modelos (aplicación principal)
  AppDelegate.hpp              # Delegate principal del editor
  StageScene.hpp               # Escena del editor con herramientas

bin/linux/                      # Binarios de salida (configurado en CMakeLists.txt)
```

## 3. Main namespaces

- **`bg2e::render`** (lib/include/bg2e/render/): Renderizado abstracto e implementación Vulkan. Gestiona Engine, Renderer, pipelines y recursos de GPU.
- **`bg2e::render::vulkan`** (lib/include/bg2e/render/vulkan/): Wrapper de bajo nivel sobre Vulkan. Gestiona Instance, Device, Swapchain, FrameResources y factories.
- **`bg2e::scene`** (lib/include/bg2e/scene/): Sistema de escena jerárquico basado en Node-Component. Gestiona Scene, Node, Components y Visitors.
- **`bg2e::ui`** (lib/include/bg2e/ui/): UI wrapper sobre Dear ImGui. UserInterface, Window, BasicWidgets.
- **`bg2e::base`** (lib/include/bg2e/base/): Tipos base: Color, Camera, MaterialAttributes, Texture.
- **`bg2e::db`** (lib/include/bg2e/db/): Carga/guardado de escenas y geometría: glTF, bg2, OBJ.
- **`bg2e::json`** (lib/include/bg2e/json/): Parser JSON para serialización de escenas y objetos.
- **`bg2e::geo`** (lib/include/bg2e/geo/): Geometría procedural: createSphere, createCube, etc.
- **`bg2e::app`** (lib/include/bg2e/app/): Application lifecycle: MainLoop, Application, InputDelegate.
- **`bg2e::math`** (lib/include/bg2e/math/): Proyecciones y utilidades matemáticas. OpticalProjection, OrthographicProjection.
- **`bg2e::manipulation`** (lib/include/bg2e/manipulation/): Herramientas de manipulación: SelectionHighlight.

## 4. Core subsystems

### Vulkan Render Subsystem
- **Engine** (lib/include/bg2e/render/Engine.hpp): Core Vulkan. Gestiona VkInstance, VkDevice, Swapchain con MSAA (VK_SAMPLE_COUNT_1_BIT por defecto), DescriptorSetAllocator, FrameResources (double/triple buffering). Inicialización: init(SDL_Window*), cleanup().
- **RenderLoop** (lib/include/bg2e/render/RenderLoop.hpp): Loop principal de renderizado. Acquire- Present cycle, delega a RenderLoopDelegate.
- **Renderer** (lib/include/bg2e/render/Renderer.hpp): Interface abstracta. build(), initScene(), draw(). Implementación concreta: RendererBasicForward.

### Scene Subsystem
- **Scene** (lib/include/bg2e/scene/Scene.hpp): Raíz del árbol de escena. Contiene rootNode(), mainCamera(), lights(). Gestiona eventos: willResize(), didDraw().
- **Node** (lib/include/bg2e/scene/Node.hpp): Nodos del árbol. Contiene components (unordered_map) y children. Soporta visitación con NodeVisitor.
- **Component** (lib/include/bg2e/scene/Component.hpp): Base para todos los componentes. Tipos: TransformComponent, CameraComponent, LightComponent, EnvironmentComponent, DrawableComponent.
- **Visitors** (lib/include/bg2e/scene/*Visitor.hpp): Patrón visitor para recorridos de escena:
  - UpdateVisitor: llama a Component.animate() y update()
  - DrawVisitor: recorre escena y añade Drawable a RenderQueue
  - InputVisitor: propaga eventos de input (mouseMove, mouseWheel)
  - RenderQueueVisitor: construye render queue desde escena

### UI Subsystem
- **UserInterface** (lib/include/bg2e/ui/UserInterface.hpp): Wrapper Dear ImGui. init(), newFrame(), draw(). Delegate: UserInterfaceDelegate (init(), drawUI()).
- **Window** (lib/include/bg2e/ui/Window.hpp): Ventanas emergentes con Dear ImGui dock space.

### Asset Subsystem
- **Texture** (lib/include/bg2e/render/Texture.hpp): Wrapper sobre VkImage y base::Texture. Soporta carga desde archivos, generación procedural (SkyDomeTextureGenerator).
- **MaterialAttributes** (lib/include/bg2e/base/MaterialAttributes.hpp): Data de material: albedo, metalness, roughness, normal, ao. UV sets (albedoUVSet, etc.), scales.
- **TextureCache** (lib/include/bg2e/utils/TextureCache.hpp): Cache de texturas reutilizables.
- **MaterialSerializer** (lib/include/bg2e/utils/MaterialSerializer.hpp): Serialización/deserialización de materiales a/from JSON.

### Database Subsystem
- **loadGltf()** (lib/include/bg2e/db/scene_gltf.hpp): Carga escenas glTF 2.0 con soporte para materiales PBR.
- **loadDrawableObj()** (lib/include/bg2e/db/mesh_obj.hpp): Carga mallas OBJ con soporte para múltiples vertex formats (MeshP, MeshPN, MeshPU, MeshPNU, MeshPNUUT).
- **loadDrawableBg2()** (lib/include/bg2e/db/mesh_bg2.hpp): Carga formato binario proprietary (.bg2) con metadata de materiales.
- **loadMeshBg2/storeMeshBg2**: Serialización binaria optimizada.

## 5. Key classes and responsibilities

### Engine Core
- **`bg2e::render::Engine`** (lib/include/bg2e/render/Engine.hpp:42)
  - responsable de crear y gestionar VkInstance, VkDevice, VkSwapchain
  - gestiona double/triple buffering con vector<FrameResources>
  - proporciona acceso a Vulkan Memory Allocator (VmaAllocator)
  - key methods: init(SDL_Window*), newFrame(), currentFrameResources()

- **`bg2e::render::vulkan::FrameResources`** (lib/include/bg2e/render/vulkan/FrameResources.hpp:33)
  - datos por-frame: commandPool, commandBuffer, semaphores (swapchainSemaphore, renderSemaphore), fence
  - CleanupManager asociado para liberar recursos temporales de frame
  - DescriptorSetAllocator por-frame para allocations efímeras

- **`bg2e::render::vulkan::Swapchain`** (lib/include/bg2e/render/vulkan/Swapchain.hpp:34)
  - gestiona VkSwapchainKHR con imágenes de color
  - soporta MSAA: _msaaImages (color buffer), _colorImages (resolve targets)
  - depthImage() y msaaDepthImage() para renderizado profundo

### Render Loop
- **`bg2e::render::RenderLoopDelegate`** (lib/include/bg2e/render/RenderLoopDelegate.hpp:28)
  - interface para custom render loop. Métodos: init(), initScene(), swapchainResized(), render()
  - render() return VkImageLayout para transición de layout final

- **`bg2e::render::RendererBasicForward`** (lib/include/bg2e/render/RendererBasicForward.hpp:44)
  - implementación concreta de Renderer para forward rendering PBR
  - pipelines: _opaquePipeline, _transparentPipeline con blend mode configurable
  - bindings: FrameDataBinding (view/proj matrix), ObjectDataBinding, EnvironmentDataBinding (lights + envmap)
  - color attachments: múltiples G-buffers para deferred post-processing futuro

- **`bg2e::render::ColorAttachments`** (lib/include/bg2e/render/ColorAttachments.hpp:30)
  - gestión de múltiples render targets
  - constructor recibe vector<VkFormat> para formats (ej: VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM)

### Scene & Components
- **`bg2e::scene::Scene`** (lib/include/bg2e/scene/Scene.hpp:31)
  - raíz de la escena. setSceneRoot(), mainCamera(), lights()
  - updateLights() rebuilds light array para shaders (se llama cuandolights cambian)

- **`bg2e::scene::Node`** (lib/include/bg2e/scene/Node.hpp:44)
  - nodo del árbol de escena. Puede tener components y children.
  - getComponent<T>() template para retrieve components
  - transform(), camera(), light() helpers
  - worldMatrix() calcula matrix de modelado con parent hierarchy

- **`bg2e::scene::Component`** (lib/include/bg2e/scene/Component.hpp:37)
  - base抽象 para todos los componentes. OwnerNode(), scene()
  - lifecycle callbacks: animate(), update(), mouseMove(), keyDown()
  - serializable: deserialize(), serialize() con JSON

- **`bg2e::scene::TransformComponent`**: Matrix de transformación (translate, rotate, scale)
- **`bg2e::scene::CameraComponent`**: Camera con projection (OpticalProjection, OrthographicProjection)
- **`bg2e::scene::LightComponent`**: Light con position, direction, intensity, color, type (point/directional/spot)
- **`bg2e::scene::EnvironmentComponent`**: Environment map (HDR equirectangular) y skybox
- **`bg2e::scene::DrawableComponent`**: Wrapper sobre Drawable (mesh + materials)

- **`bg2e::scene::Drawable`** (lib/include/bg2e/scene/Drawable.hpp)
  - geometría renderizable con múltiples materials (por submesh)
  - mesh_ (geo::Mesh*) y materials_ vector<MaterialAttributes>
  - load(engine) carga VBO/IBO en GPU

- **`bg2e::scene::InputVisitor`** (lib/include/bg2e/scene/InputVisitor.hpp)
  - propaga eventos de input a componentes
  - mouseMove(), mouseButton*, mouseWheel()

### Factory Patterns
- **`bg2e::render::vulkan::factory::GraphicsPipeline`** (lib/include/bg2e/render/vulkan/factory/GraphicsPipeline.hpp:33)
  - builder para graphics pipelines
  - addShader(), setInputState<MeshT>(), setColorAttachmentFormat(), build()
  - template input state con Mesh::bindingDescription() y attributeDescriptions()

- **`bg2e::render::vulkan::factory::ShaderModule`** (lib/include/bg2e/render/vulkan/factory/ShaderModule.hpp:31)
  - carga shaders desde .spv (VK_SHADER_STAGE_VERTEX_BIT, FRAGMENT_BIT, etc.)
  - PlatformTools::shaderPath() para ruta por defecto

- **`bg2e::render::vulkan::factory::Sampler`** (lib/include/bg2e/render/vulkan/factory/Sampler.hpp:31)
  - VkSamplerCreateInfo builder. build() con min/mag filter, address mode

### Data Bindings (Shader Uniforms)
- **`bg2e::scene::vk::FrameDataBinding`** (lib/include/bg2e/scene/vk/FrameDataBinding.hpp)
  - binds viewMatrix y projMatrix (set=0, binding=0 SceneData uniform)
- **`bg2e::scene::vk::ObjectDataBinding`** (lib/include/bg2e/scene/vk/ObjectDataBinding.hpp)
  - binds modelMatrix y PBRMaterialData (set=1, binding=0)
- **`bg2e::scene::vk::EnvironmentDataBinding`** (lib/include/bg2e/scene/vk/EnvironmentDataBinding.hpp)
  - binds lights array, irradianceMap, prefilteredEnvMap, brdfLUT (set=2)
- **`bg2e::scene::vk::LightDataBinding`**: Light array binding (separate para deferred)

### UI & App
- **`bg2e::ui::UserInterface`** (lib/include/bg2e/ui/UserInterface.hpp:30)
  - wrapper Dear ImGui. init(), newFrame(), draw()
  - delegate pattern: UserInterfaceDelegate (init(), drawUI())

- **`bg2e::ui::Window`** (lib/include/bg2e/ui/Window.hpp)
  - ventana emergente con ImGui window flags

- **`bg2e::app::Application`** (lib/include/bg2e/app/Application.hpp:28)
  - interfaz para apps. setRenderDelegate(), setInputDelegate(), setUiDelegate()
  - delegates heredan de RenderLoopDelegate, InputDelegate, UserInterfaceDelegate

- **`bg2e::app::MainLoop`** (lib/include/bg2e/app/MainLoop.hpp:32)
  - loop principal. run(Application*), initWindowConfig()
  - event processing y frame timing

### Base Types
- **`bg2e::base::Color`** (lib/include/bg2e/base/Color.hpp:26)
  - struct RGBA float. Static methods: Black(), White(), Red(), etc.
- **`bg2e::base::Camera`** (lib/include/bg2e/base/Camera.hpp:30)
  - projection matrix. setProjection(), projectionMatrix()
- **`bg2e::base::MaterialAttributes`** (lib/include/bg2e/base/MaterialAttributes.hpp:35)
  - PBR material data. albedo(), metalness(), roughness(), normalTexture()
  - UV set indices (albedoUVSet, etc.) y scales
- **`bg2e::base::Texture`**: Textura base con sampler config (wrap mode, filter)

### Manipulation
- **`bg2e::manipulation::SelectionHighlight`** (lib/include/bg2e/manipulation/SelectionHighlight.hpp:32)
  - highlight de selección con line intensity
  - NodeVisitor para recorrer escena y draw

## 6. Runtime flow

1. **Application init** (`main()` en examples/01_setup/src/main.cpp:142):
   - MainLoop mainLoop("bundle id")
   - Application::init() configura delegates

2. **MainLoop.run(application)** (lib/include/bg2e/app/MainLoop.hpp):
   - initWindowConfig() configura SDL_Window (title, size, fullscreen)
   - initFrameResources() inicializa DescriptorSetAllocator
   - Frame loop: acquireAndPresent(), update(delta), render()

3. **Engine initialization** (lib/include/bg2e/render/Engine.hpp:45):
   - init(SDL_Window*): cria VkInstance (con validation layers), VkSurfaceKHR, VkPhysicalDevice, VkDevice
   - createSwapchain() con MSAA (VK_SAMPLE_COUNT_1_BIT)
   - createFrameResources(): double buffering

4. **RenderLoop init** (lib/include/bg2e/render/RenderLoop.hpp:36):
   - RenderLoopDelegate::init(engine) configura engine pointer
   - initFrameResources() inicializa descriptor pool
   - initScene(): crea pipelines, carga textures y 3D models

5. **Frame loop**:
   - newFrame(): acquire next swapchain image
   - RenderLoopDelegate::initScene() (si primera frame)
   - update(delta): llama a Component::update()
   - render(): dibuja scene y UI
   - present: muestra frame en swapchain

6. **Render path** (RendererBasicForward::draw):
   - begin command buffer
   - transition color image to COLOR_ATTACHMENT_OPTIMAL
   - cmdBeginRendering() con VkRenderingInfo (color + depth attachments)
   - bind pipeline (opaquePipeline o transparentPipeline)
   - bind descriptor sets: FrameData, ObjectData, EnvironmentData
   - draw meshes (DrawableComponent)
   - render UI overlay con ImGui
   - end rendering
   - transition to PRESENT_SRC_KHR

7. **Cleanup**: cleanup() libera pipelines, descriptor pools, swapchain, VkDevice, VkInstance

## 7. Rendering architecture

### Backend Vulkan
- **Physical Device**: Selección de GPU con Vulkan SDK validation layers (habilitable)
- **Device**: Queue gráfica y present queue (mismo family en大多数 GPUs)
- **Swapchain**: Imágenes color con MSAA (VK_SAMPLE_COUNT_1_BIT por defecto)
- **Depth buffer**: Depth attachment con VkFormat (ej: VK_FORMAT_D32_SFLOAT)

### Pipeline Organization
- **Graphics Pipeline Builder** (factory/GraphicsPipeline.hpp):
  - addShader(): carga shaders .spv
  - setInputState<MeshT>() con vertex binding/attribute descriptions
  - setColorAttachmentFormat(): R16G16B16A16_SFLOAT, R8G8B8A8_UNORM
  - setDepthFormat(): depth buffer format
  - enableBlendingAlphablend() para transparent objects

- **VK Pipeline Layouts**:
  - Set=0, Binding=0: SceneData (viewMatrix, projMatrix)
  - Set=1, Binding=0: PBRObjectData (modelMatrix, material)
  - Set=1, Binding 1-5: albedoTex, normalTex, metallicTex, roughnessTex, aoTex
  - Set=2, Binding=0: LightData (lights array)
  - Set=2, Bindings 1-3: irradianceMap, prefilteredEnvMap, brdfLUT
  - Push constant: gamma, brightness, contrast, exposure

### Materials & Shaders
- **Shader Organization** (shaders/src/):
  - basic_forward.vert.glsl: vertex shader con PBR data
  - basic_forward.frag.glsl: fragment shader Cook-Torrance BRDF

- **PBR Shaders** (shaders/src/lib/pbr.glsl):
  - fresnelSchlick(): F0 mix entre dielectric/metal
  - distributionGGX(): NDF con roughness
  - geometrySmith(): shadowing-masking
  - calcRadiancePoint(): point light radiance
  - calcAmbientLight(): environment lighting (irradiance + specular)

- **Material Uniforms** (shaders/src/lib/uniforms.glsl:24):
  ```glsl
  struct PBRMaterialData {
      vec4 albedo;
      vec4 fresnelTint;
      vec2 albedoScale, normalScale, metalnessScale, roughnessScale;
      float metalness, roughness;
      int albedoUVSet, normalUVSet, metalnessUVSet, roughnessUVSet, aoUVSet;
      float sheenIntensity;
      vec4 sheenColor;
  };
  ```

- **Light Uniforms** (shaders/src/lib/uniforms.glsl:47):
  ```glsl
  struct Light {
      vec3 position;
      float intensity;
      vec4 color;
      vec3 direction;
      int type; // POINT, DIRECTIONAL, SPOT
  };
  const int LIGHT_COUNT = 8;
  ```

### Features Implemented
- **Multiple Color Attachments**: Render targets para G-buffers (R16G16B16A16_SFLOAT, R8G8B8A8_UNORM)
- **PBR Material System**: Albedo, metalness, roughness, normal map, AO
- **Environment Mapping**: HDR equirectangular textures → cubemap (irradiance + specular)
- **IBL (Image-Based Lighting)**: BRDF LUT, prefiltered environment map
- **MSAA**: Anti-aliasing en render targets (configurable sample count)
- **Dynamic Light Count**: Up to 8 lights in single pass
- **Bias/Contrast/Gamma Correction**: Push constant para tone mapping

### Shaders por use case
- **basic_forward.***: Forward PBR rendering
- **skybox_renderer.***: Skybox renderizado con envmap
- **irradiance_map_renderer.frag.glsl**: Generate irradiance map from HDR environment
- **specular_reflection_renderer.ffrag.glsl**: Generate prefiltered specular map
- **brdf_lut.comp.glsl**: Compute shader para BRDF integration LUT

## 8. Examples as usage reference

### examples/01_setup
- Setup básico Vulkan: Instance, Device, Swapchain creation
- Render loop con RenderLoopDelegate
- GraphicsPipeline builder: addShader(), setColorAttachmentFormat(), build()
- Pipeline layout con VkDescriptorSetLayout

### examples/02_ui
- UserInterface initialization y initImGui()
- drawUI() callback para Dear ImGui widgets
- Window emergente con options (noClose, minWidth, minHeight)

### examples/10_scene_objects
- Escena compleja con múltiples objetos y components
- Multiple data bindings:
  - FrameDataBinding (view/proj matrix)
  - ObjectDataBinding (model + material)
  - EnvironmentDataBinding (lights + envmap)
- SkyDomeTextureGenerator para procedural skybox
- ColorAttachments con múltiples render targets

### examples/13_pbr_demo
- Renderizador PBR completo:
  - Scene con EnvironmentComponent (HDR environment maps: mirrored_hall, theater, autumn_field, gothic_manor)
  - Lights (point lights con intensity)
  - Materiales: metallic-roughness workflow
  - UI para editar metalness, roughness, albedo en tiempo real
- OrbitCameraComponent para interacción 3D

### examples/debug-app
- App settings path (PlatformTools::settingsPath())
- Preferences: clave/valor storage
- PreferencesStore singleton para persistent settings
- GPUSelectionDialog para selección de device Vulkan

### examples/14_bg2_model_load
- Carga de modelos .bg2 (formato proprietary)
- loadDrawableBg2() con metadata de materiales

## 9. Conventions and patterns

### Naming Conventions
- **Classes**: PascalCase (Engine, Renderer, Node, Component)
- **Methods**: camelCase (init(), update(), draw(), setAlbedo())
- **Constants**: SCREAMING_SNAKE_CASE (LIGHT_COUNT, VK_SAMPLE_COUNT_1_BIT)
- **Members**: _prefijo para atributos privados (_engine, _sceneRoot)
- **Templates**: T suffix (RendererT en DefaultRenderLoopDelegate)

### Factory Patterns
- **vulkan::factory namespace**: Builder pattern para recursos Vulkan:
  - ShaderModule::loadFromSPV()
  - GraphicsPipeline builder
  - PipelineLayout builder
  - Sampler builder
- **Texture static methods**: blackTexture(), whiteTexture(), colorTexture()
- **Scene geometry**: createSphere(), createCube(), createPlane()

### Registry Patterns
- **ComponentFactoryRegistry** (lib/include/bg2e/scene/ComponentFactoryRegistry.hpp): Registry para component types
- **PreferencesStore**: Singleton pattern (PreferencesStore::instance())

### Visitor Pattern
- **NodeVisitor base class** (lib/include/bg2e/scene/NodeVisitor.hpp):
  - visit(), didVisit()
  - Concrete visitors:
    - UpdateVisitor: update() + animate()
    - DrawVisitor: build RenderQueue
    - InputVisitor: propaga eventos input
    - ResizeViewportVisitor: cameras resize

### Ownership & Lifetime
- **enable_shared_from_this<Node>**: Node ownership with shared_ptr
- **Weak pointers en Components**: _owner (weak ptr para evitar circular references)
- **CleanupManager**: Push lambdas para cleanup automático de recursos Vulkan
- **FrameResources flushFrameData()**: Libera temporary resources cada frame

### Serialization
- **JSON format** (json::JsonNode, JsonParser):
  - Scene serialize/deserialize
  - Component serialize/deserialize
  - MaterialAttributes JSON storage
- **Binary .bg2 format**: Optimized mesh + material serialization

### Delegate Pattern
- **RenderLoopDelegate**: Custom render loop behavior
- **InputDelegate**: Input event handling (mouse, keyboard)
- **UserInterfaceDelegate**: UI initialization y draw callbacks
- **Application**: Configura delegates

### Template Patterns
- **DefaultRenderLoopDelegate<RendererT>**: Generic render loop con concrete renderer template parameter
- **loadMeshObj<MeshT>()**: Template para múltiples vertex formats (MeshP, MeshPN, MeshPNUUT)
- **Node::getComponent<T>()**: Template para component lookup

### Resource Management
- **DescriptorSetAllocator**: Pool reutilizable de descriptor sets (evita allocations frecuentes)
- **FrameResources cleanupManager()**: Cleanup por-frame automático
- **TextureCache**: Reutilización de texturas idénticas

### Shader组织
- **#include "lib/*.glsl"**: Shared GLSL libraries (pbr.glsl, uniforms.glsl)
- **push_constant**: Tone mapping parameters (gamma, brightness, contrast, exposure)

## 10. Important files index

| File | Reason |
|------|--------|
| `lib/include/bg2e.hpp` | Header principal que incluye todos los módulos |
| `lib/include/bg2e/common.hpp` | Platform defines y BG2E_API para DLL export |
| `lib/include/bg2e/render/Engine.hpp` | Core Vulkan: Instance, Device, Swapchain, FrameResources |
| `lib/include/bg2e/render/Renderer.hpp` | Interface abstracta para renderers |
| `lib/include/bg2e/render/RendererBasicForward.hpp` | Renderizador PBR forward implementado |
| `lib/include/bg2e/render/RenderLoop.hpp` | Loop principal de renderizado |
| `lib/include/bg2e/render/vulkan/Instance.hpp` | Vulkan Instance wrapper |
| `lib/include/bg2e/render/vulkan/Device.hpp` | Vulkan Device y queues wrapper |
| `lib/include/bg2e/render/vulkan/Swapchain.hpp` | Swapchain con MSAA |
| `lib/include/bg2e/render/vulkan/FrameResources.hpp` | Data por-frame (command buffers, semaphores) |
| `lib/include/bg2e/render/vulkan/DescriptorSetAllocator.hpp` | Pool reutilizable de descriptor sets |
| `lib/include/bg2e/render/vulkan/factory/GraphicsPipeline.hpp` | Builder para graphics pipelines |
| `lib/include/bg2e/render/vulkan/factory/ShaderModule.hpp` | Carga shaders .spv |
| `lib/include/bg2e/render/vulkan/factory/Sampler.hpp` | Builder para VkSampler |
| `lib/include/bg2e/scene/Scene.hpp` | Raíz de la escena |
| `lib/include/bg2e/scene/Node.hpp` | Nodos del árbol de escena |
| `lib/include/bg2e/scene/Component.hpp` | Base para componentes |
| `lib/include/bg2e/scene/UpdateVisitor.hpp` | Visitor para update loop |
| `lib/include/bg2e/scene/DrawVisitor.hpp` | Visitor para render queue construction |
| `lib/include/bg2e/ui/UserInterface.hpp` | Wrapper Dear ImGui |
| `lib/include/bg2e/base/MaterialAttributes.hpp` | PBR material data struct |
| `lib/include/bg2e/db/scene_gltf.hpp` | Carga escenas glTF 2.0 |
| `lib/include/bg2e/db/mesh_obj.hpp` | Carga geometría OBJ |
| `lib/include/bg2e/db/mesh_bg2.hpp` | Carga formato binario proprietary (.bg2) |
| `lib/include/bg2e/app/Application.hpp` | Interface para apps |
| `lib/include/bg2e/app/MainLoop.hpp` | Main loop principal |
| `examples/01_setup/src/main.cpp` | Example minimal setup Vulkan render loop |
| `examples/13_pbr_demo/src/main.cpp` | Demo completa PBR con UI |
| `shaders/src/basic_forward.vert.glsl` | Vertex shader PBR forward |
| `shaders/src/basic_forward.frag.glsl` | Fragment shader Cook-Torrance BRDF |
| `shaders/src/lib/pbr.glsl` | PBR functions (fresnel, GGX, Smith) |
| `shaders/src/lib/uniforms.glsl` | Uniform structs (Material, Light) |
| `apps/model_edit/src/main.cpp` | Entry point para editor de modelos |

## 11. Engine capabilities and current limitations

### Current capabilities
- Native C++ graphics engine designed for professional applications, tools, editors and high-quality rendering workflows.
- Vulkan backend with dynamic rendering (`VK_KHR_dynamic_rendering`), without relying on traditional `VkRenderPass`.
- Forward renderer with Physically Based Rendering (PBR).
- Image-Based Lighting (IBL) support:
  - irradiance maps
  - prefiltered specular environment maps
  - BRDF LUT
- Material system with support for:
  - albedo
  - metalness
  - roughness
  - normal maps
  - ambient occlusion
  - UV set selection
  - texture scale parameters
- Scene graph based on hierarchical `Node` + `Component`.
- Scene serialization/deserialization using JSON for scenes/components and binary `.bg2` for optimized 3D assets.
- glTF and OBJ loading support.
- Multiple render targets / color attachments infrastructure already present in the renderer.
- MSAA support integrated in the Vulkan backend.
- Dear ImGui integration for native tools and editors.
- Cross-platform native support on macOS, Windows and Linux.
- Engine architecture suitable for:
  - content creation tools
  - scene editors
  - asset pipelines
  - high-quality native visualization

### Current limitations
- The current high-quality path is still based on forward rendering; deferred rendering is not yet implemented in this C++ API.
- Real-time shadows are not yet implemented in the current renderer.
- Screen-space effects such as SSAO and SSR are not yet present in the current Vulkan implementation, even though they were explored in earlier OpenGL-based versions.
- HDR output is planned but not yet part of the current production renderer.
- Ray tracing support is not yet implemented, although the engine is beginning to evolve toward hardware capability detection and future RT-based rendering paths.
- Advanced lighting features are still incomplete; directional and spot light workflows are planned as part of the renderer evolution.
- Mac support is constrained by the Vulkan-on-Metal translation layer ecosystem; hardware ray tracing is currently limited by backend/platform support rather than only by engine design.
- The engine is not designed around videogame-oriented workflows; its priorities are native integration, authoring tools, asset processing and professional visualization.
- The current renderer remains a valid fallback path for devices that do not support future advanced features such as ray tracing.

### Intended evolution
- Introduce hardware capability detection for advanced rendering features, especially ray tracing-related Vulkan extensions and feature flags.
- Keep the current forward PBR renderer as a stable compatibility path.
- Add a separate high-end rendering path for advanced devices, prioritizing image quality over broad hardware compatibility.
- Focus future renderer evolution on modern techniques rather than legacy-compatible intermediate solutions.

## 12. Design intent

- This C++ engine is intended for native professional applications, not for videogames.
- The renderer is expected to evolve toward high-quality rendering, but that goal is secondary to native tooling and creation workflows.
- The current forward PBR renderer is considered a valid baseline for authoring tools and unsupported hardware.
- Future rendering work prioritizes modern high-end techniques over broader compatibility when both goals conflict.
- The C++ API is fully independent from the TypeScript API and should be understood as a separate native codebase with its own architecture and priorities.