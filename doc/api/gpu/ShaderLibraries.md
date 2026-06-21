# Shader Libraries

**Header:** `<bg2e/gpu/ShaderLib.hpp>` (included by `<bg2e/gpu/all.hpp>`)  
**Namespace:** `bg2e::gpu`

A **ShaderLib** is a directory of compiled shaders that groups all backends
together under a common naming convention. `gpu::ShaderLib` wraps that directory
and lets application code load shaders without ever writing a backend branch:

```cpp
// One call works on both Vulkan and Metal.
auto shaderLib = backend->createShaderLib(shaderBasePath / "my_shaders");
auto vs = shaderLib->vertex("cube", device.get());
auto fs = shaderLib->fragment("cube", device.get());
```

---

## The problem ShaderLib solves

Without ShaderLib, loading a shader for two backends requires an `if/else` block
that leaks backend knowledge into application code:

```cpp
std::shared_ptr<gpu::ShaderModule> vs;
std::shared_ptr<gpu::ShaderModule> fs;

if (backendType == gpu::BackendType::Vulkan)
{
    auto vsPath = (shaderPath / "cube.vert.spv").string();
    auto fsPath = (shaderPath / "cube.frag.spv").string();
    vs = device->createShaderModule({ vsPath, "vertMain", gpu::ShaderStage::Vertex,   "Cube VS" });
    fs = device->createShaderModule({ fsPath, "fragMain", gpu::ShaderStage::Fragment, "Cube FS" });
}
else  // Metal
{
    auto vsPath = (shaderPath / "cube.vert.metallib").string();
    auto fsPath = (shaderPath / "cube.frag.metallib").string();
    vs = device->createShaderModule({ vsPath, "vertMain", gpu::ShaderStage::Vertex,   "Cube VS" });
    fs = device->createShaderModule({ fsPath, "fragMain", gpu::ShaderStage::Fragment, "Cube FS" });
}
```

The same block with ShaderLib:

```cpp
auto shaderLib = backend->createShaderLib(shaderPath);
auto vs = shaderLib->vertex("cube", device.get());
auto fs = shaderLib->fragment("cube", device.get());
```

The backend selection, file extension, and entry point are all resolved
internally.

---

## File-system conventions

### Source ShaderLib

A source ShaderLib is a flat directory containing one file per shader stage per
backend. Both backends live side by side and are distinguished only by file
extension (`.glsl` for GLSL/Vulkan, `.metal` for Metal).

```
my_shader_lib/
  lib/               <-- optional include files used by the GLSL sources
  cube.vert.glsl
  cube.vert.metal
  cube.frag.glsl
  cube.frag.metal
  skybox.vert.glsl
  skybox.vert.metal
  skybox.frag.glsl
  skybox.frag.metal
```

### Compiled ShaderLib

After building, the compiled ShaderLib mirrors the same flat layout. Vulkan
SPIR-V (`.spv`) and Metal libraries (`.metallib`) share the same directory,
distinguished only by extension:

```
<bin>/shaders/my_shader_lib/
  cube.vert.spv
  cube.vert.metallib
  cube.frag.spv
  cube.frag.metallib
  skybox.vert.spv
  skybox.vert.metallib
  skybox.frag.spv
  skybox.frag.metallib
```

### Stage extension mapping

The stage portion of a filename determines both the shader type and the entry
point name:

| Stage extension | `ShaderStage`            | Entry point (Metal)  |
|-----------------|--------------------------|----------------------|
| `.vert`         | `ShaderStage::Vertex`    | `vertMain`           |
| `.frag`         | `ShaderStage::Fragment`  | `fragMain`           |
| `.comp`         | `ShaderStage::Compute`   | `compMain`           |
| `.rgen`         | *(ray generation)*       | `rgenMain`           |
| `.rmiss`        | *(ray miss)*             | `rmissMain`          |
| `.rchit`        | *(ray closest-hit)*      | `rchitMain`          |
| `.rahit`        | *(ray any-hit)*          | `rahitMain`          |
| `.rint`         | *(ray intersection)*     | `rintMain`           |

---

## Entry point conventions

Entry points are backend-specific: GLSL uses `main`, Metal uses `<stage>Main`.

| Backend | Language | Entry point name   |
|---------|----------|--------------------|
| Vulkan  | GLSL     | `main`             |
| Metal   | MSL      | `<stage>Main` (e.g. `vertMain`, `fragMain`) |

The `<stage>Main` naming convention is specific to Metal shaders. GLSL requires
the entry point to be named `main` in both source and compiled SPIR-V output.

**GLSL note:** GLSL requires the entry point to be named `main`. The
`compile_shaders_shaderlib()` CMake function no longer passes `-e <stage>Main`
for GLSL — the compiled SPIR-V retains `main` as the entry point. ShaderLib
maps the `<stage>Main` API call to `main` for Vulkan/SPIR-V. The production
`compile_shaders()` function is not affected and continues to use `main` for the
existing `bg2e::render` shaders.

**Metal note:** MSL does not allow `main` as a function name (it conflicts with
the C/C++ program entry point). The source function is named `vertMain`,
`fragMain`, etc. directly.

### GLSL example

```glsl
// cube.vert.glsl — entry point is main (GLSL standard)
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec2 fragUV;

layout(set = 0, binding = 0) uniform CameraUBO { mat4 projectionView; } camera;
layout(set = 1, binding = 0) uniform ModelUBO  { mat4 model;          } object;

void main()
{
    gl_Position = camera.projectionView * object.model * vec4(inPosition, 1.0);
}
// Compiled SPIR-V entry point: "main" (unchanged from source)
```

### MSL example

```metal
// cube.vert.metal  — function named vertMain directly
#include <metal_stdlib>
using namespace metal;

struct VertexIn  { float3 position [[attribute(0)]]; };
struct VertexOut { float4 position [[position]]; };

vertex VertexOut vertMain(VertexIn in [[stage_in]])
{
    VertexOut out;
    out.position = float4(in.position, 1.0);
    return out;
}
// Metallib entry point: "vertMain"
```

---

## API reference

### `gpu::ShaderLib`

```cpp
class BG2E_API ShaderLib {
public:
    ShaderLib(const std::filesystem::path& basePath, BackendType backendType);

    std::shared_ptr<ShaderModule> vertex(
        const std::string& shaderName,
        Device*            device,
        const std::string& debugName = "");

    std::shared_ptr<ShaderModule> fragment(
        const std::string& shaderName,
        Device*            device,
        const std::string& debugName = "");

    std::shared_ptr<ShaderModule> compute(
        const std::string& shaderName,
        Device*            device,
        const std::string& debugName = "");

    std::shared_ptr<ShaderModule> rayGeneration(
        const std::string& shaderName,
        Device*            device,
        const std::string& debugName = "");

    std::shared_ptr<ShaderModule> miss(
        const std::string& shaderName,
        Device*            device,
        const std::string& debugName = "");

    std::shared_ptr<ShaderModule> closestHit(
        const std::string& shaderName,
        Device*            device,
        const std::string& debugName = "");
};
```

#### Constructor

```cpp
ShaderLib(const std::filesystem::path& basePath, BackendType backendType);
```

`basePath` is the directory that directly contains the compiled shader files.
`backendType` controls which file extension (`.spv` or `.metallib`) and which
entry point name convention is used.

Prefer `Backend::createShaderLib()` over calling the constructor directly, as it
automatically passes the current backend type.

#### `vertex / fragment / compute`

```cpp
std::shared_ptr<ShaderModule> vertex(
    const std::string& shaderName,
    Device*            device,
    const std::string& debugName = "");
```

Resolves the full file path and entry point from `shaderName` and the stage,
then calls `device->createShaderModule()`. The returned `ShaderModule` is owned
by the caller and must be cleaned up with `ShaderModule::cleanup()` (or via
`CleanupManager::push()`, which silently accepts `nullptr` for stages that are
not applicable on a given backend).

| Parameter    | Description |
|--------------|-------------|
| `shaderName` | Base name without stage or extension, e.g. `"cube"`. Resolves to `cube.vert.spv` or `cube.vert.metallib`. |
| `device`     | Logical device used to create the shader module.   |
| `debugName`  | Optional label shown in validation layers and Xcode GPU Capture. Defaults to `"<shaderName> vertex shader"`. |

### `gpu::Backend::createShaderLib()`

```cpp
std::unique_ptr<ShaderLib> createShaderLib(
    const std::filesystem::path& basePath) const;
```

Factory method on `gpu::Backend`. Creates a `ShaderLib` pre-configured for the
active backend. Implemented in the base class using `backendType()`:

```cpp
// Simplified view of the base-class implementation
std::unique_ptr<ShaderLib> Backend::createShaderLib(
    const std::filesystem::path& basePath) const
{
    return std::make_unique<ShaderLib>(basePath, backendType());
}
```

#### `rayGeneration / miss / closestHit`

```cpp
std::shared_ptr<ShaderModule> rayGeneration(
    const std::string& shaderName,
    Device*            device,
    const std::string& debugName = "");

std::shared_ptr<ShaderModule> miss(
    const std::string& shaderName,
    Device*            device,
    const std::string& debugName = "");

std::shared_ptr<ShaderModule> closestHit(
    const std::string& shaderName,
    Device*            device,
    const std::string& debugName = "");
```

Load ray tracing shader modules. The file resolution follows the same pattern
as `vertex()` / `fragment()` / `compute()`:

| Method | Stage extension | Vulkan file | Metal file | Vulkan entry | Metal entry |
|--------|----------------|-------------|------------|--------------|-------------|
| `rayGeneration()` | `.rgen` | `.rgen.spv` | `.rgen.metallib` | `main` | `rgenMain` |
| `miss()` | `.rmiss` | `.rmiss.spv` | `.rmiss.metallib` | `main` | `rmissMain` |
| `closestHit()` | `.rchit` | `.rchit.spv` | `.rchit.metallib` | `main` | `rchitMain` |

**Metal note:** On Metal, `miss()` and `closestHit()` return `nullptr` when the
`.rmiss.metallib` or `.rchit.metallib` file does not exist. This is expected
because Metal handles miss/hit behavior internally in the compute kernel. The
`RayTracingPipeline` accepts null pointers for these shaders without error.

`rayGeneration()` always throws if the file is missing (the rgen shader is
required on both backends).

---

## Build system integration

ShaderLibs for `gpu::` examples are compiled with `compile_shaders_shaderlib()`
instead of the standard `compile_shaders()`. For Metal, shaders are compiled with
`xcrun -sdk iphoneos metallib` (or `macosx`). For Vulkan, GLSL shaders are
compiled with `glslang` as usual, using `main` as the entry point (no `-e` flag
needed — matches production shader compilation).

### CMakeLists.txt pattern for a ShaderLib example

```cmake
set(APP_TARGET_NAME my_gpu_example)
set(APP_SHADERS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(APP_SHADERS_DST "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders")
set(METAL_SHADERS_DST "${PRODUCT_DIR}/${APP_TARGET_NAME}_resources/app_shaders/metal")

# Register the target (no SHADERS_SRC — handled explicitly below)
bundle_app_sdl(TARGET_NAME ${APP_TARGET_NAME})

# Compile GLSL shaders for the gpu:: API (entry point: main, same as production)
compile_shaders_shaderlib(${APP_TARGET_NAME} ${VULKAN_SDK}
    "${APP_SHADERS_SRC}" "${APP_SHADERS_DST}")
bundle_resources(TARGET_NAME ${APP_TARGET_NAME}
    SRC_PATH ${APP_SHADERS_DST}
    SUBPATH "shaders/${APP_TARGET_NAME}")

# Compile Metal shaders (macOS only)
if(APPLE)
    compile_metal_shaders(${APP_TARGET_NAME}
        "${APP_SHADERS_SRC}" "${METAL_SHADERS_DST}")
    bundle_resources(TARGET_NAME ${APP_TARGET_NAME}
        SRC_PATH ${METAL_SHADERS_DST}
        SUBPATH "shaders/${APP_TARGET_NAME}")
endif()
```

The `SHADERS_SRC` parameter is intentionally omitted from `bundle_app_sdl` so
that `compile_shaders()` (which uses `main` as the entry point) is not invoked.
`compile_shaders_shaderlib()` is called directly afterwards.

Both GLSL (`.glsl`) and Metal (`.metal`) sources sit in the same flat `shaders/`
directory. `compile_shaders_shaderlib()` picks up only `*.glsl` files;
`compile_metal_shaders()` picks up only `*.metal` files.

---

## Before / after comparison

The following examples show the same shader-loading code first written without
ShaderLib (`device->createShaderModule()` with manual backend detection) and
then rewritten to use `ShaderLib`.

### Vertex + fragment shaders

**Before — manual backend detection**

```cpp
auto shaderPath = base::PlatformTools::shaderPath() / "gpu_uniform_buffers";

std::shared_ptr<gpu::ShaderModule> vs;
std::shared_ptr<gpu::ShaderModule> fs;

if (backendType == gpu::BackendType::Vulkan)
{
    auto vsPath = (shaderPath / "cube.vert.spv").string();
    auto fsPath = (shaderPath / "cube.frag.spv").string();
    vs = device->createShaderModule({ vsPath, "vertMain", gpu::ShaderStage::Vertex,   "Cube VS" });
    fs = device->createShaderModule({ fsPath, "fragMain", gpu::ShaderStage::Fragment, "Cube FS" });
}
else  // Metal
{
    auto vsPath = (shaderPath / "cube.vert.metallib").string();
    auto fsPath = (shaderPath / "cube.frag.metallib").string();
    vs = device->createShaderModule({ vsPath, "vertMain", gpu::ShaderStage::Vertex,   "Cube VS" });
    fs = device->createShaderModule({ fsPath, "fragMain", gpu::ShaderStage::Fragment, "Cube FS" });
}

cleanup.push(vs);
cleanup.push(fs);
```

**After — ShaderLib**

```cpp
auto shaderLib = backend->createShaderLib(
    base::PlatformTools::shaderPath() / "gpu_uniform_buffers");

auto vs = shaderLib->vertex("cube", device.get());
auto fs = shaderLib->fragment("cube", device.get());

cleanup.push(vs);
cleanup.push(fs);
```

### Vertex + fragment + compute shaders

**Before**

```cpp
auto shaderPath = base::PlatformTools::shaderPath() / "gpu_simple_triangle";

std::shared_ptr<gpu::ShaderModule> vs, fs, cs;

if (backendType == gpu::BackendType::Vulkan)
{
    vs = device->createShaderModule({
        (shaderPath / "triangle.vert.spv").string(),
        "vertMain", gpu::ShaderStage::Vertex, "Triangle VS" });
    fs = device->createShaderModule({
        (shaderPath / "triangle.frag.spv").string(),
        "fragMain", gpu::ShaderStage::Fragment, "Triangle FS" });
    cs = device->createShaderModule({
        (shaderPath / "gradient.comp.spv").string(),
        "compMain", gpu::ShaderStage::Compute, "Gradient CS" });
}
else  // Metal
{
    vs = device->createShaderModule({
        (shaderPath / "triangle.vert.metallib").string(),
        "vertMain", gpu::ShaderStage::Vertex, "Triangle VS" });
    fs = device->createShaderModule({
        (shaderPath / "triangle.frag.metallib").string(),
        "fragMain", gpu::ShaderStage::Fragment, "Triangle FS" });
    cs = device->createShaderModule({
        (shaderPath / "gradient.comp.metallib").string(),
        "compMain", gpu::ShaderStage::Compute, "Gradient CS" });
}
```

**After**

```cpp
auto shaderLib = backend->createShaderLib(
    base::PlatformTools::shaderPath() / "gpu_simple_triangle");

auto vs = shaderLib->vertex("triangle",  device.get());
auto fs = shaderLib->fragment("triangle", device.get());
auto cs = shaderLib->compute("gradient",  device.get());
```

### Using a custom debug name

```cpp
auto vs = shaderLib->vertex("cube", device.get(), "PBR cube vertex shader");
auto fs = shaderLib->fragment("cube", device.get(), "PBR cube fragment shader");
```

When `debugName` is omitted, the label defaults to `"<shaderName> vertex shader"`,
`"<shaderName> fragment shader"`, etc.

---

## Complete example (example 07 — uniform buffers)

This is the actual shader-loading section from
`examples/gpu/07_uniform_buffers/src/main.cpp`, which uses `ShaderLib`:

```cpp
// device was already created from backend, physicalDevice, and surface.
// shaderBasePath resolves to bin/<platform>/shaders/ at runtime.

auto shaderLib = backend->createShaderLib(
    base::PlatformTools::shaderPath() / "gpu_uniform_buffers");

auto vs = shaderLib->vertex("cube", device.get());
auto fs = shaderLib->fragment("cube", device.get());

cleanup.push(vs);
cleanup.push(fs);

// vs and fs are then passed to GraphicsPipelineDescription:
gpu::GraphicsPipelineDescription pipelineDesc{};
pipelineDesc.vertexShader   = vs.get();
pipelineDesc.fragmentShader = fs.get();
pipelineDesc.layout         = graphicsLayout.get();
// ...
auto pipeline = device->createGraphicsPipeline(pipelineDesc);
cleanup.push(pipeline);
```

The corresponding ShaderLib directory at runtime looks like:

```
bin/<platform>/shaders/gpu_uniform_buffers/
   cube.vert.spv          <-- Vulkan (entry point: main)
   cube.frag.spv          <-- Vulkan (entry point: main)
   cube.vert.metallib     <-- Metal  (entry point: vertMain)
   cube.frag.metallib     <-- Metal  (entry point: fragMain)
```

---

## Relationship to `ShaderModule`

`ShaderLib` does not replace `ShaderModule`. It is a loader that sits one level
above it. Internally, every `vertex()`, `fragment()`, and `compute()` call
resolves to a single `device->createShaderModule()` invocation, and the returned
`std::shared_ptr<ShaderModule>` is the same object you would get from that call
directly.

```
Backend::createShaderLib(basePath)
    -> ShaderLib(basePath, backendType)
        -> ShaderLib::vertex("cube", device)
            -> device->createShaderModule({
                    basePath / "cube.vert.spv",  // or .metallib
                    "main" (Vulkan) / "vertMain" (Metal),
                    ShaderStage::Vertex,
                    "cube vertex shader"
                })
            -> shared_ptr<ShaderModule>        <-- same type as always
```

`ShaderModule` cleanup, `CleanupManager` integration, and pipeline attachment
all work exactly as before.

---

## See also

- [`ShaderModule`](ShaderModule.md) — the underlying per-stage shader wrapper
- [`Device`](Device.md) — `createShaderModule()` reference
- [`Backend`](Backend.md) — `createShaderLib()` and `backendType()`
- [`GraphicsPipeline`](GraphicsPipeline.md) — how shader modules attach to pipelines
