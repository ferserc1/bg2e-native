# bg2e-native Module Dependencies

## Project Overview

bg2e-native es un motor gráfico en C++ (business grade graphic engine) que usa Vulkan como backend de renderizado. El código está organizado en 11 módulos internos bajo `lib/include/bg2e/` con una arquitectura de capas clara.

## Arquitectura por Capas (9 niveles)

```
Capa 0: Foundation     → math, base, common       (sin dependencias internas)
Capa 1: Data Structures → json, geo                (dependen de Capa 0)
Capa 2: Rendering       → render (+ vulkan/*)      (dependen de Capas 0-1)
Capa 3: Scene Graph     → scene (+ vk/*)           (dependen de Capas 0-2)
Capa 4: Data Loading    → db                       (dependen de Capas 0-3)
Capa 5: Interaction     → manipulation             (dependen de Capas 0-3)
Capa 6: Utilities       → utils                    (dependen de Capas 0-2)
Capa 7: Application     → app                      (dependen de Capas 0-6)
Capa 8: User Interface  → ui                       (dependen de Capas 0-7)
```

## Inventario de Módulos

| Módulo | Headers | Dependencias internas | Capa |
|--------|---------|----------------------|------|
| **math** | 4 (base, projections, tools) | Ninguna | 0 |
| **base** | 8 (Color, Image, Texture, Camera, Light, Log, MaterialAttributes, PlatformTools) | math/projections, json/JsonNode | 0 |
| **json** | 3 (JsonNode, JsonParser, JsonToken) | common, math/base, base/Color | 1 |
| **geo** | 8 (Mesh, Vertex, cube, plane, sphere, cylinder, cone, modifiers) | common, math/base (transitiva) | 1 |
| **render** | 21 + sub-módulos vulkan/factory, vulkan/geo, vulkan/rt, vulkan/macros, uniforms | common, vulkan/*, scene/Node, base/Texture, math/base | 2 |
| **scene** | 24 + scene/vk (5 data bindings) | common, math/projections, json/JsonNode, render/Engine, app/KeyEvent | 3 |
| **db** | 5 (image, mesh_bg2, mesh_obj, scene, scene_gltf) | base/Image, geo/Mesh, scene/Drawable, render/Engine | 4 |
| **manipulation** | 4 (PickSelectionVisitor, SelectableComponent, SelectionManager, SelectionHighlight) | scene/*, render/Engine, math/projections, vulkan/Image | 5 |
| **utils** | 3 (TextureCache, utils, MaterialSerializer) | render/Texture, base/Texture, render/Engine, base/MaterialAttributes | 6 |
| **app** | 11 (Application, InputManager, MainLoop, Preferences, Dialogs...) | common, base/*, ui/* | 7 |
| **ui** | 16 (UserInterface, Windows, Widgets, Editors...) | app/*, scene/*, render/Engine | 8 |

## Grafo de Dependencias (edges principales)

```
math ──────────────→ (sin dependencias)
  ↑
base ──────────────→ math/projections, json/JsonNode
  ↑       ↑
json ─────┘       geo ──→ common, math (transitiva)
  ↑                       ↑
utils ────────────────→ render, base
  ↑                        ↑
manipulation ─────────→ scene, render
  ↑                            ↑
db ────────────────────→ scene, render, geo
  ↑                            ↑
app ───────────────────→ ui (y scene vía Component)
  ↑                            ↑
ui ────────────────────→ app, scene, render
```

## Dependencias detalladas por módulo

### math (Capa 0 - Foundation)
- **Sin dependencias internas**. Es la base pura del motor.
- `projections.hpp` incluye `bg2e/json/JsonNode` (para serialización de cámaras).

### base (Capa 0 - Foundation)
- `Camera.hpp` → math/projections, json/JsonNode
- `Light.hpp` → base/Color, json/JsonNode
- `MaterialAttributes.hpp` → common, base/Color, base/Texture
- `Texture.hpp` → base/Image, base/Color

### json (Capa 1 - Data Structures)
- `JsonNode.hpp` → common, math/base, base/Color

### geo (Capa 1 - Data Structures)
- `Mesh.hpp` → common, geo/Vertex
- Primitives (cube, plane, sphere, cylinder, cone) → common, geo/Mesh

### render (Capa 2 - Rendering Engine)
- `Engine.hpp` → vulkan/Instance, Command, Swapchain, extensions, CleanupManager, FrameResources, Surface, PhysicalDevice, Device
- `Renderer.hpp` → scene/Node, scene/Scene, vulkan/Image, FrameResources, Engine
- `Texture.hpp` → vulkan/Image, base/Texture, base/Color
- `MaterialBase.hpp` → base/MaterialAttributes, render/Texture
- `RenderQueue.hpp` → MaterialBase, math/base, scene/Drawable
- `CubemapRenderer.hpp` → Engine, Texture, vulkan/Buffer, DescriptorSetAllocator, vulkan/geo/Mesh
- `SkyboxRenderer.hpp` → CubemapRenderer, base/PlatformTools

### scene (Capa 3 - Scene Graph)
- `Component.hpp` → common, math/projections, app/KeyEvent, json/JsonNode
- `Node.hpp` → Component, NodeVisitor, json/JsonNode, TransformComponent, DrawableComponent, CameraComponent, EnvironmentComponent, LightComponent
- `Drawable.hpp` → scene/Mesh, base/MaterialAttributes, render/Engine, MaterialBase, vulkan/geo/Mesh, geo/modifiers, Component, vulkan/rt/RayTracingMesh
- `scene/vk/all.hpp` → EnvironmentDataBinding, ObjectDataBinding, FrameDataBinding

### db (Capa 4 - Data Loading)
- `image.hpp` → common, base/Image, base/Texture
- `mesh_bg2.hpp` → common, base/MaterialAttributes, geo/Mesh, scene/Drawable, render/Engine
- `mesh_obj.hpp` → common, geo/Mesh, scene/Drawable, render/Engine
- `scene_gltf.hpp` → common, scene/Node, render/Engine

### manipulation (Capa 5 - Interaction)
- `PickSelectionVisitor.hpp` → scene/NodeVisitor, scene/DrawableComponent
- `SelectableComponent.hpp` → common, scene/Component
- `SelectionManager.hpp` → render/Engine, scene/Drawable, scene/CameraComponent, scene/Scene, math/projections, scene/Node, PickSelectionVisitor, vulkan/Image
- `SelectionHighlight.hpp` → scene/NodeVisitor, math/all, scene/Drawable, render/Engine

### utils (Capa 6 - Utilities)
- `TextureCache.hpp` → render/Texture, base/Texture, render/Engine
- `MaterialSerializer.hpp` → common, base/MaterialAttributes

### app (Capa 7 - Application)
- `Application.hpp` → common, base/*, ui/*
- `InputManager.hpp` → InputDelegate, KeyEvent

### ui (Capa 8 - User Interface)
- `UserInterface.hpp` → common, app/KeyEvent, scene/*, render/Engine

## Dependencias Circulares Detectadas

| Ciclo | Severidad | Detalle |
|-------|-----------|---------|
| **scene::Component ↔ app::KeyEvent** | ALTA | `Component.hpp:23` incluye `<bg2e/app/KeyEvent.hpp>`, invirtiendo la jerarquía esperada (app debería depender de scene, no al revés). |
| **db → render/Engine + scene::Drawable** | MEDIA | La capa de datos depende del motor de renderizado. Razonable para carga en runtime pero crea acoplamiento fuerte. |
| **manipulation → vulkan::Image** | BAJA | Manipulación depende directamente de detalles Vulkan, haciendo el módulo no portable a otras APIs. |
| **scene::Drawable → vulkan::geo::Mesh + rt::RayTracingMesh** | BAJA | El scene graph depende de tipos Vulkan-specific. |

## Third-Party Dependencies

| Biblioteca | Uso |
|-----------|-----|
| **GLM** | math/base, geo/Vertex -- matemáticas vectoriales/matriciales |
| **Vulkan SDK** | render/vulkan/* -- API gráfica principal |
| **VMA** (Vulkan Memory Allocator) | render/vulkan/common.hpp -- gestión de memoria GPU |
| **SDL2** | render/Engine.hpp -- creación de ventanas |
| **GTK3/Wayland** (Linux) | nfd_gtk.cpp, UI scale detection |
| **std::filesystem / iostream** | Múltiples módulos -- operaciones de archivo y logging |

## Reglas para Modificar el Código

1. **Nunca romper la jerarquía de capas**: un módulo de capa N solo debe depender de módulos de capa <= N.
2. **Evitar incluir headers Vulkan en scene/**: si es posible, usar interfaces abstractas para mantener el scene graph independiente del backend gráfico.
3. **Mover KeyEvent a base/**: para resolver el ciclo scene ↔ app, `KeyEvent` debería vivir en un módulo de capa inferior (base o common).
4. **No añadir dependencias db → render**: la carga de datos debería poder funcionar sin un motor de renderizado inicializado (usar factory patterns o interfaces abstractas).
5. **Los módulos de Capa 0 (math, base) deben permanecer sin dependencias internas**: son la fundación del motor.

## Estructura de Directorios Clave

```
lib/include/bg2e/              # Headers principales del motor
  all.hpp                      # Incluye todos los módulos (entry point)
  common.hpp                   # Platform defines y BG2E_API macros
  math/                        # Capa 0: proyecciones, utilidades (GLM)
  base/                        # Capa 0: Color, Camera, Texture, MaterialAttributes
  json/                        # Capa 1: parser JSON para serialización
  geo/                         # Capa 1: geometría procedural (Mesh, primitives)
  render/                      # Capa 2: Engine, Renderer, RenderLoop, Textures
    vulkan/                    # Backend Vulkan de bajo nivel
      factory/                 # Builders para pipelines, shaders, samplers
      geo/                     # Vulkan mesh wrappers
      rt/                      # Ray tracing meshes
      macros/                  # Helper macros C++
    uniforms/                  # Structs PBR para shaders
  scene/                       # Capa 3: Node, Component, Drawable, Visitors
    vk/                        # Data bindings para Vulkan (uniforms)
  manipulation/                # Capa 5: selección, highlight, pick visitor
  utils/                       # Capa 6: TextureCache, MaterialSerializer
  db/                          # Capa 4: carga glTF, OBJ, formato .bg2
  app/                         # Capa 7: Application, MainLoop, Input, Preferences
  ui/                          # Capa 8: UserInterface, Windows, Widgets, Editors

lib/src/bg2e/                  # Implementación .cpp de todos los módulos
apps/model_edit/               # Editor de modelos (app principal)
examples/                      # 15 ejemplos + debug-app
shaders/src/                   # Shaders GLSL (PBR forward, skybox, IBL)
```

## Entry Points

- **Header principal**: `lib/include/bg2e.hpp` → incluye `bg2e/all.hpp`
- **App principal**: `apps/model_edit/src/main.cpp`
- **Ejemplo mínimo**: `examples/01_setup/src/main.cpp`
- **Demo PBR completa**: `examples/13_pbr_demo/src/main.cpp`
