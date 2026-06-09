# Implementation Plan — GPU Pipelines & Metal Shaders

> Plan to extend `bg2e::gpu` from the current clear-loop to a minimal geometry-drawing
> application (a `vertex_id`-generated triangle) on **both Vulkan and Metal**, plus a
> minimal compute pipeline. **This is a planning document only — no code is implemented here.**

---

## 1. General objective

Advance the `bg2e::gpu` multi-backend API from the working clear loop
(`examples/gpu/04_clear_loop`) to a working triangle and a minimal compute dispatch,
introducing the pipeline abstractions needed for real rendering:

- Backend-independent **shader module / shader library** abstraction.
- Backend-independent **`gpu::PipelineLayout`**, **`gpu::GraphicsPipeline`**, **`gpu::ComputePipeline`**.
- Minimal **command buffer** extensions: `bindPipeline`, `draw`, `dispatch`, and a foundation for `pushConstants`.
- Separated shader build infrastructure: **GLSL → SPIR-V** (Vulkan) and **MSL → `.metallib`** (Metal, macOS only).
- A new example **`examples/gpu/05_simple_triangle`**, copied from `04_clear_loop`, that draws a triangle in both backends and runs a minimal compute dispatch.

The first visual milestone is a triangle whose vertices are generated **in the vertex shader from `vertex_id`** (no vertex buffers).

### Out of scope (this plan)

- Vertex/index buffers, uniform buffers, textures, samplers, storage buffers, descriptor sets / argument buffers (only the *foundation* for these is prepared; see resource-binding roadmap below).
- Any new shader added to the **engine** shader library (`shaders/src/**`). New shaders for the triangle live **only** in the example.
- A single shared shader source. The design keeps **two shader sets**: GLSL/SPIR-V for Vulkan, MSL/metallib for Metal.

---

## 2. Current state detected in the project

### 2.1 `gpu` namespace (already working)

- Abstract interfaces in `lib/include/bg2e/gpu/`: `Backend`, `Instance`, `PhysicalDevice`,
  `Device`, `Surface` (`WindowSurface` / `OffscreenSurface`), `SurfaceFrame`, `Queue`,
  `CommandBuffer`, `Image`, plus POD types in `Common.hpp`
  (`Size2D`, `Size3D`, `PixelFormat`, `Color`, `ImageLayout`, `BackendType`).
- Vulkan implementation in `lib/{include,src}/bg2e/gpu/vk/`.
- Metal implementation in `lib/{include,src}/bg2e/gpu/metal/` (guarded by `BG2E_IS_MAC`; stubbed otherwise).
- `gpu::Factory` selects the backend; `gpu::Backend` exposes factory methods
  (`createPhysicalDevice`, `createDevice`, `createWindowSurface`, `createOffscreenSurface`)
  and `sharedInstance()` / `windowType()`.
- `gpu::Device` exposes `graphicsQueue()` / `presentQueue()` / `transferQueue()`.
- `gpu::Queue` exposes `createCommandBuffer()` and `submit()`.
- `gpu::CommandBuffer` currently supports: `begin`, `end`, `transition`, `beginRendering`,
  `endRendering`, `clearColor`, `clearDepth`, `isValid`.
- `examples/gpu/04_clear_loop` runs the full animated clear loop on both backends.

### 2.2 Native handle access (needed by the pipeline steps)

- `vk::Device::handle()` → `VkDevice`; `vk::Device::allocator()` → `VmaAllocator`.
- `metal::Device::handle()` → `MTL::Device*` (alias `DeviceHandle`).
- `vk::CommandBuffer::handle()` → `VkCommandBuffer`; `metal::CommandBuffer::handle()` → `MTL::CommandBuffer*`.
- `vk::Image` / `metal::Image` expose the native image/view/texture used by `beginRendering`.

### 2.3 Two important rendering-scope behaviours (design-critical)

- **Vulkan `CommandBuffer` defers rendering.** `beginRendering()` only records intent;
  `flushPendingRendering()` emits `vkCmdBeginRendering` lazily on the first `end()`/`endRendering()`.
  → A draw call must force `flushPendingRendering()` *before* recording `vkCmdBindPipeline` / `vkCmdDraw`.
- **Metal `CommandBuffer` builds the encoder inside `endRendering()`.** `beginRendering()`
  only builds the `MTL::RenderPassDescriptor`; the `MTL::RenderCommandEncoder` is created
  and immediately ended in `endRendering()`.
  → To record draws, the encoder must be created *before* the first draw and kept alive until `endRendering()`.

Both behaviours are addressed in the **`bindPipeline`** step (`012`) with a shared rule:
the encoder/rendering scope must be *materialised* on first pipeline bind or draw, and clear-only
frames (example 04) must keep working unchanged.

### 2.4 Shader build infrastructure (current)

- `cmake/utils.cmake`:
  - `build_shaders(TARGET, VULKAN_SDK, SRC, DST)` — `POST_BUILD` glob compile.
  - `compile_shaders(TARGET, VULKAN_SDK, SRC, DST)` — `add_custom_command` per `*.glsl`, with RT-stage detection (`--target-env vulkan1.2`); creates `${TARGET}_shaders` target.
  - `bundle_app` / `bundle_app_sdl` accept optional `SHADERS_SRC` to compile per-example shaders into `shaders/<target>`.
- `lib/CMakeLists.txt`: `SRC_SHADER_DIR = shaders/src`, `DST_SHADER_DIR = ${PRODUCT_DIR}/shaders`; calls `compile_shaders` and exports `BG2E_SHADER_DIR`.
- Standalone scripts `shaders/build.sh` / `shaders/build.bat` glob `*.glsl` (used outside CMake).
- `shaders/src/lib/` holds shared GLSL `#include`s; **glslang resolves includes relative to it** — it must keep the same relative position to the GLSL sources.
- `examples/02_compute_shader/` is the reference for **per-example** shaders (`shaders/src/*.glsl` + `bundle_app(... SHADERS_SRC ...)`).
- **No Metal shader compilation infrastructure exists yet.**

### 2.5 CMake / platform facts

- Generators are platform-locked (macOS = Xcode, Windows = VS2022, Linux = Ninja).
- `BG2E_IS_MAC` guards Metal code; `metal-cpp` is vendored at `lib/third_party/metal-cpp`; Metal framework is already linked on Apple.
- Auto-glob includes any new `.hpp/.cpp` under `lib/`, `examples/`, etc. — **no CMake edit needed for new engine source files** (CMake edits are only needed for shader tooling and example wiring).

---

## 3. Architecture summary of the additions

```
                       gpu::Device  (factory: createShaderModule / createPipelineLayout
                          │           / createGraphicsPipeline / createComputePipeline)
        ┌─────────────────┼──────────────────────────────┐
        ▼                 ▼                               ▼
 gpu::ShaderModule   gpu::PipelineLayout        gpu::GraphicsPipeline / gpu::ComputePipeline
   ├ vk: VkShaderModule   ├ vk: VkPipelineLayout    ├ vk: VkPipeline (+ bind point)
   └ metal: MTL::Function └ metal: logical metadata  └ metal: MTL::RenderPipelineState /
     (+ MTL::Library)        (push-constant/binding      MTL::ComputePipelineState
                              map; no MTL object)

 gpu::CommandBuffer  +=  bindPipeline / draw / dispatch / pushConstants
   ├ vk:    flush rendering, then vkCmdBindPipeline / vkCmdDraw / vkCmdDispatch / vkCmdPushConstants
   └ metal: ensure encoder, then setRenderPipelineState / drawPrimitives /
            (compute encoder) dispatchThreadgroups / setVertexBytes·setFragmentBytes·setBytes
```

### 3.1 Backend mapping (target)

| Concept                | Vulkan                                              | Metal                                                       |
|------------------------|-----------------------------------------------------|-------------------------------------------------------------|
| Shader module          | `VkShaderModule` (from `.spv`)                       | `MTL::Function` from `MTL::Library` (from `.metallib`)       |
| Pipeline layout        | `VkPipelineLayout`                                   | logical metadata (binding/push-constant map; no MTL object) |
| Graphics pipeline      | `VkPipeline` + `VK_PIPELINE_BIND_POINT_GRAPHICS`    | `MTL::RenderPipelineState` (`MTL::RenderPipelineDescriptor`) |
| Compute pipeline       | `VkPipeline` + `VK_PIPELINE_BIND_POINT_COMPUTE`     | `MTL::ComputePipelineState`                                  |
| Push / small constants | `vkCmdPushConstants`                                 | `setVertexBytes` / `setFragmentBytes` / `setBytes`          |
| Draw                   | `vkCmdDraw`                                          | `drawPrimitives`                                            |
| Dispatch               | `vkCmdDispatch`                                      | compute encoder `dispatchThreadgroups`                      |

### 3.2 Shader output layout (target)

```
shaders/src/glsl/**.glsl     # GLSL sources (moved from shaders/src/**), incl. lib/
shaders/src/metal/           # MSL sources (created empty in this plan; no engine MSL added)
shaders/        (SPIR-V .spv) # Vulkan engine output — UNCHANGED location
shaders/metal/  (.metallib)   # Metal engine output — prepared, not yet populated
```

Example `05_simple_triangle` keeps **its own** shader sources (GLSL + MSL) under the example,
compiled and bundled per-example (like `02_compute_shader`).

---

## 4. Cross-cutting conventions (apply to every step)

1. **Namespace boundary.** All engine API changes live in `bg2e::gpu` (and `gpu::vk` / `gpu::metal`).
   No edits outside `bg2e::gpu` except: CMake, shader build tooling/scripts, example wiring, build aux files.
2. **Compile-safe extension pattern (keeps each step buildable).** When adding a new factory
   method to an abstract base (`gpu::Device`, `gpu::CommandBuffer`), add it as a **non-pure virtual
   with a default that throws "not implemented"** (or returns `nullptr`), *not* as a pure virtual.
   - The base step then compiles with both backends inheriting the default.
   - Each backend step *overrides* the method.
   - This satisfies rule 5 (base before implementation) without breaking the build between steps.
3. **Metal isolation.** All Metal code stays under `BG2E_IS_MAC`; all Metal CMake stays under `if(APPLE)`.
   Linux/Windows must never search for or invoke Metal tools.
4. **Vulkan compatibility.** The existing `shaders/` SPIR-V output location and all current engine
   shaders keep working unchanged after step 001.
5. **Atomicity.** Each step is the smallest useful unit and leaves the project compiling
   (compilation/verification is performed by the developer, not by this plan).
6. **No premature abstraction.** Resource sets / argument buffers are *not* built here; only the
   pipeline-layout/push-constant foundation that they will later extend.

---

## 5. Ordered list of steps

| #   | File | Title | Type |
|-----|------|-------|------|
| 001 | `001-shader-tools-folder-refactor.md` | Refactor shader tooling to `shaders/src/glsl/` | Build infra |
| 002 | `002-metal-shader-cmake-tools.md` | Metal shader compile tools (macOS only) + `shaders/metal` | Build infra |
| 003 | `003-base-shader-module.md` | Base `gpu::ShaderModule` + `ShaderStage` types | Base API |
| 004 | `004-vulkan-shader-module.md` | Vulkan `vk::ShaderModule` (`VkShaderModule`) | Backend |
| 005 | `005-metal-shader-module.md` | Metal `metal::ShaderModule` (`MTL::Library`/`MTL::Function`) | Backend |
| 006 | `006-base-pipeline-layout.md` | Base `gpu::PipelineLayout` + layout description | Base API |
| 007 | `007-vulkan-pipeline-layout.md` | Vulkan `vk::PipelineLayout` (`VkPipelineLayout`) | Backend |
| 008 | `008-metal-pipeline-layout.md` | Metal `metal::PipelineLayout` (logical metadata) | Backend |
| 009 | `009-base-graphics-pipeline.md` | Base `gpu::GraphicsPipeline` + description | Base API |
| 010 | `010-vulkan-graphics-pipeline.md` | Vulkan `vk::GraphicsPipeline` (`VkPipeline` graphics) | Backend |
| 011 | `011-metal-graphics-pipeline.md` | Metal `metal::GraphicsPipeline` (`MTL::RenderPipelineState`) | Backend |
| 012 | `012-command-buffer-bind-pipeline.md` | `cmd->bindPipeline()` + rendering-scope materialisation | Command buffer |
| 013 | `013-command-buffer-draw.md` | `cmd->draw()` | Command buffer |
| 014 | `014-example-05-scaffold.md` | Copy `04` → `05_simple_triangle`; dual shader sources + CMake | Example |
| 015 | `015-example-05-triangle.md` | Create pipeline, bind, `draw(3)` → triangle renders | Example |
| 016 | `016-base-compute-pipeline.md` | Base `gpu::ComputePipeline` + description | Base API |
| 017 | `017-vulkan-compute-pipeline.md` | Vulkan `vk::ComputePipeline` (`VkPipeline` compute) | Backend |
| 018 | `018-metal-compute-pipeline.md` | Metal `metal::ComputePipeline` (`MTL::ComputePipelineState`) | Backend |
| 019 | `019-command-buffer-dispatch.md` | `cmd->dispatch()` + compute encoder handling | Command buffer |
| 020 | `020-example-05-compute-validation.md` | Minimal compute dispatch validation in `05` | Example |
| 021 | `021-command-buffer-push-constants.md` | `cmd->pushConstants()` foundation + triangle color | Command buffer |

---

## 6. Dependencies between steps

```
001 ──► 002                      (folder refactor before adding metal tools)
003 ──► 004, 005                 (base shader module before backends)
006 ──► 007, 008                 (base layout before backends)
003,006 ──► 009 ──► 010, 011     (graphics pipeline needs shader module + layout)
010,011 ──► 012 ──► 013          (bind before draw)
002,004,005,010,011,013 ──► 014 ──► 015   (example needs tooling + pipelines + draw)
003,006 ──► 016 ──► 017, 018     (compute pipeline base before backends)
017,018 ──► 019                  (dispatch needs compute pipelines)
015,019 ──► 020                  (compute validation after triangle + dispatch)
012/013 ──► 021                  (push constants extend the bind/draw path)
```

Notes:
- 001 and 002 (build infra) are independent of the C++ pipeline steps and can be done first.
- Within each `base → vk → metal` triad, the two backend steps are independent of each other.
- Metal backend steps (005, 008, 011, 018) only become *runtime-relevant* on macOS but must
  compile (as guarded stubs) on Linux/Windows.

---

## 7. Expected state at the end of each phase

| Phase | Steps | Project state at the end |
|-------|-------|--------------------------|
| Shader infra | 001–002 | Shaders moved to `shaders/src/glsl/`; SPIR-V still in `shaders/`; Metal compile function exists (macOS-only); `shaders/metal` prepared. Engine + all examples build & run exactly as before. |
| Shader modules | 003–005 | `gpu::ShaderModule` creatable on both backends; nothing uses it yet. Builds on all platforms. |
| Pipeline layout | 006–008 | `gpu::PipelineLayout` creatable on both backends. Builds. |
| Graphics pipeline | 009–011 | `gpu::GraphicsPipeline` creatable on both backends. Builds; not yet bound. |
| Command buffer (draw) | 012–013 | `cmd->bindPipeline()` and `cmd->draw()` exist; clear loop (04) still works; rendering scope materialised correctly on both backends. |
| Triangle example | 014–015 | `examples/gpu/05_simple_triangle` renders a `vertex_id` triangle on Vulkan and Metal. |
| Compute | 016–019 | `gpu::ComputePipeline` + `cmd->dispatch()` on both backends. Builds & runs. |
| Compute validation | 020 | `05` performs a minimal compute dispatch (need not be visible). |
| Push constants | 021 | `cmd->pushConstants()` foundation; triangle color driven by a push constant on both backends. |

---

## 8. General validation criteria

For **every** step:
- Engine library `bg2e` and all existing examples (especially `examples/gpu/04_clear_loop`) compile.
- On macOS, both Vulkan and Metal backends compile; on Linux/Windows, Metal code compiles as guarded stubs and no Metal tool is invoked.
- No symbol or behaviour change outside `bg2e::gpu` except the allowed build/tooling/example files.

For the **functional milestones**:
- After 015: launching `gpu_simple_triangle` and selecting either backend shows a triangle.
- After 019/020: the compute pipeline is created and dispatched without validation errors.
- After 021: changing the push-constant color value changes the triangle color on both backends.

Verification (compiling and running) is performed by the developer; this plan does not compile code.

---

## 9. Open design decisions (resolved here, revisited per step)

- **Who creates pipelines?** → `gpu::Device` gains factory methods (`createShaderModule`,
  `createPipelineLayout`, `createGraphicsPipeline`, `createComputePipeline`). Rationale: pipeline
  creation needs the native device (`VkDevice` / `MTL::Device*`), which `Device` owns, and this
  mirrors `Queue::createCommandBuffer()`. *Alternative considered:* methods on `Backend` taking a
  `Device*`, or a dedicated `gpu::PipelineFactory`. Documented in steps 003/006/009/016.
- **Shader source per backend.** → The example passes a backend-specific file path to
  `createShaderModule` (Vulkan `.spv`, Metal `.metallib` + function name). No common source.
- **Pipeline needs attachment formats.** → `GraphicsPipelineDescription` carries color/depth
  `PixelFormat`s (required by `VkPipelineRenderingCreateInfo` and `MTL::RenderPipelineDescriptor`).
  The example reads them from the surface frame images.
- **Metal pipeline layout has no native object.** → `metal::PipelineLayout` stores binding/push
  metadata only; it exists to keep a symmetric API and to host the future argument-buffer path.
