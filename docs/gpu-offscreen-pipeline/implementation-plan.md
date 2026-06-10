# Implementation Plan — GPU Offscreen Pipeline

> Plan to make everything already implemented in `bg2e::gpu` (graphics + compute
> pipelines, draw, dispatch, push constants — see `docs/gpu-pipelines-and-metal-shaders/`)
> also work on **offscreen surfaces** on **both Vulkan and Metal**, and to validate it
> with a CLI example that renders the `05_simple_triangle` command buffer into an
> offscreen image and **writes the result to disk**.
> **This is a planning document only — no code is implemented here.**

---

## 1. General objective

The pipeline work (`gpu::GraphicsPipeline`, `gpu::ComputePipeline`, `cmd->bindPipeline/draw/dispatch/pushConstants`)
is complete and validated **only against `WindowSurface`** (example `05_simple_triangle`).
This plan completes the **offscreen** path so the *same* command-buffer recording runs
against an `OffscreenSurface` and the rendered image can be read back to CPU memory and saved.

Concretely, the plan delivers:

1. A **surface refactoring** that introduces backend-common intermediate classes
   `vk::Surface` and `metal::Surface`, hosting the code shared by window *and* offscreen
   surfaces of each backend (today duplicated between `WindowSurface` and `OffscreenSurface`).
2. A **public, backend-agnostic synchronous command submission** primitive
   (`gpu::Device::immediateSubmit`) that takes a closure over the abstract
   `gpu::CommandBuffer`. It records, submits and blocks until the GPU finishes. It is needed
   now to copy GPU images back to host memory deterministically, but is designed as a
   general-purpose primitive for future high-level, backend-independent code (uploads,
   one-shot transitions, mip generation, etc.).
3. A new **image read-back API** on `gpu::Image` (`readPixelsRGBA8`) with concrete
   implementations in `vk::Image` and `metal::Image`. This is the most involved part:
   it adds a GPU→CPU staging path on both backends.
4. A new CLI example `examples/gpu/06_offscreen_triangle` that reuses the
   `05_simple_triangle` pipeline/shaders, renders one frame into an offscreen color
   image, reads the pixels into a `std::vector<uint8_t>`, and saves them with
   `bg2e::db::saveImage` next to the working directory.

### Reference (stable engine API)

The stable Vulkan renderer already does this with
`bg2e::render::vulkan::Image::readPixelsRGBA8(...)` (staging buffer + `immediateSubmit` +
`vkCmdCopyImageToBuffer`) and `bg2e::render::vulkan::Command::immediateSubmit(...)`
(dedicated pool + fence + `vkWaitForFences`). The new `gpu` versions follow the same shape
but are backend-agnostic at the public level and live entirely inside `bg2e::gpu`.

### Out of scope (this plan)

- No new resource binding (vertex/index/uniform/storage buffers, samplers, descriptor sets,
  argument buffers). The triangle is still generated from `vertex_id`.
- No general-purpose `gpu::Buffer` abstraction. The Vulkan staging buffer is created inline
  with VMA; the Metal staging buffer is an inline `MTL::Buffer`.
- No depth read-back. Only RGBA8 color read-back is implemented (the only format the
  offscreen surface uses by default: `PixelFormat::R8G8B8A8_UNORM`).
- No change to the window/swapchain present path beyond the surface refactoring.

---

## 2. Current state detected in the project

### 2.1 What already works

- `gpu::OffscreenSurface` (base) and `vk::OffscreenSurface` / `metal::OffscreenSurface`
  already create a color (and optional depth) render target and expose `beginFrame()` →
  `SurfaceFrame` with `colorImage()` / `depthImage()`. `present()` / `endFrame()` are no-ops.
- Example `examples/gpu/03_offscreen_device` already proves offscreen **device creation**
  works (no window, no present queue).
- The color image created by `vk::Image::buildTargetImage` already includes
  `VK_IMAGE_USAGE_TRANSFER_SRC_BIT` (read-back ready). The Metal target texture uses
  `MTL::StorageModePrivate` with `RenderTarget | ShaderRead` usage (blit-source capable).
- The whole pipeline/draw/dispatch/push-constants command recording from
  `05_simple_triangle` is **surface-type agnostic** — it operates on the `SurfaceFrame`
  images, not on window specifics.

### 2.2 What is missing for offscreen to be usable end-to-end

1. **No read-back API.** There is no way to copy a `gpu::Image` into a `std::vector<uint8_t>`.
2. **No synchronous submit.** `Queue::submit()` for an offscreen frame (no present frame)
   submits with a `VK_NULL_HANDLE` fence and never waits; Metal `commit()` does not wait.
   A deterministic read-back needs a synchronous GPU→CPU copy (`immediateSubmit`).
3. **No offscreen rendering example** and no `out.*` image writing path.
4. **Code duplication** between `WindowSurface` and `OffscreenSurface` of each backend
   (depth-image management, cached device pointer, frame storage). The user requested an
   intermediate `vk::Surface` / `metal::Surface` to host this common code.

### 2.3 Submission / synchronization facts (design-critical)

- `vk::Queue::submit()` (`lib/src/bg2e/gpu/vk/Queue.cpp`): when the command buffer has **no
  present frame** it does `queueSubmit2(..., VK_NULL_HANDLE)` — fire-and-forget, no fence.
  → The example must call `device->waitIdle()` after submit before reading back, **or** the
  read-back itself must run on a synchronous path. This plan uses both:
  `waitIdle()` after the render submit, and a self-contained `immediateSubmit` inside the
  read-back for the copy.
- `metal::Queue::submit()` calls `commit()` only. The read-back's `immediateSubmit` must use
  `waitUntilCompleted()`.
- `vk::Device::create()` already retrieves the graphics queue and builds per-queue command
  pools; a **dedicated immediate pool + fence** must be added there (mirroring the stable
  `render::vulkan::Command`).

### 2.4 Surface class hierarchy (today)

```
gpu::Surface (abstract)
├── gpu::WindowSurface            (marker: isOffscreen()==false)
│     ├── vk::WindowSurface
│     └── metal::WindowSurface
└── gpu::OffscreenSurface         (marker: isOffscreen()==true, holds Size2D ctor)
      ├── vk::OffscreenSurface
      └── metal::OffscreenSurface
```

The factory (`gpu::Backend`) returns `std::unique_ptr<gpu::WindowSurface>` and
`std::unique_ptr<gpu::OffscreenSurface>` — these typed return values must be preserved.

---

## 3. Target architecture

### 3.1 Surface refactoring (intermediate backend class)

Insert a backend-common class shared by *both* window and offscreen surfaces of the same
backend, while keeping the `gpu::WindowSurface` / `gpu::OffscreenSurface` markers (and the
typed factory) intact. Because the common class must cross the Window/Offscreen split, this
is a diamond and is resolved with **virtual inheritance** of `gpu::Surface`:

```
                 gpu::Surface  (virtual base)
        ┌────────────┼─────────────┬───────────────┐
        │            │             │               │
 gpu::WindowSurface  gpu::OffscreenSurface     vk::Surface / metal::Surface
   (virtual Surface) (virtual Surface)         (virtual Surface; common code)
        └─────┬──────┘                               │
              │  ┌──────────────────────────────────┘
              ▼  ▼
   vk::WindowSurface  : public gpu::WindowSurface,    public vk::Surface
   vk::OffscreenSurface: public gpu::OffscreenSurface, public vk::Surface
   (same for metal::)
```

`vk::Surface` / `metal::Surface` hold the code common to both surface kinds:
the cached down-cast device pointer, the depth-image member and its
create/resize/release helpers, and (later) the shared read-target plumbing.

*Alternative considered (documented in step 001):* flatten the markers — drop
`gpu::WindowSurface`/`gpu::OffscreenSurface` from the concrete ancestry and change the factory
return type to `gpu::Surface`. Simpler (no diamond) but changes the public factory signature
and loses the compile-time window/offscreen distinction. **Virtual inheritance is recommended.**

### 3.2 Read-back + synchronous submit

```
 gpu::Device  (public, backend-agnostic — non-pure virtual, default throws)
   + virtual void immediateSubmit(std::function<void(gpu::CommandBuffer* cmd)>&& fn)
        records one command buffer, runs fn(cmd) over the ABSTRACT command buffer,
        submits on the graphics queue, and blocks until completion.
        ├ vk::Device    : reset dedicated pool+buffer+fence, wrap native cmd in vk::CommandBuffer,
        │                 begin → fn → end → queueSubmit2(fence) → vkWaitForFences
        └ metal::Device : graphics-queue command buffer wrapped in metal::CommandBuffer,
                          begin → fn → end → commit → waitUntilCompleted

 gpu::Image
   + virtual void readPixelsRGBA8(std::vector<uint8_t>& out, ImageLayout currentLayout)
        device->immediateSubmit([&](gpu::CommandBuffer* cmd) {
            cmd->transition(this, ImageLayout::TransferSrc);   // ABSTRACT
            <backend copy command via cmd down-cast to native handle>
            cmd->transition(this, currentLayout);              // ABSTRACT
        });
        ├ vk::Image    : vkCmdCopyImageToBuffer → VMA staging (GPU_TO_CPU) → mapped ptr → memcpy
        └ metal::Image : blit copyFromTexture:toBuffer: → shared MTL::Buffer → contents() → memcpy
```

`immediateSubmit` is a **public, backend-agnostic** method on the abstract `gpu::Device`
(non-pure virtual with a default that throws "not implemented", following the compile-safe
extension pattern). The closure receives the **abstract** `gpu::CommandBuffer*`, so future
high-level code can record backend-independent commands (`transition`, `beginRendering`,
`draw`, `dispatch`, …) inside it.

The read-back is the one current user that also needs a *copy* command, which is deliberately
not on the abstract `gpu::CommandBuffer` (it would require a `gpu::Buffer` abstraction, out of
scope). The backend `Image` therefore records the abstract transitions through `cmd`, then
down-casts `cmd` to its native command buffer (`vk::CommandBuffer::handle()` /
`metal::CommandBuffer::handle()`) to record the single native copy command. This keeps
`immediateSubmit` itself fully generic while letting the low-level read-back reach the native
handle when it must.

### 3.3 Backend mapping (read-back)

| Concept                 | Vulkan                                            | Metal                                              |
|-------------------------|---------------------------------------------------|----------------------------------------------------|
| `immediateSubmit`       | dedicated pool + fence + `vkWaitForFences`        | graphics-queue cmd + `commit` + `waitUntilCompleted` |
| Closure parameter       | `gpu::CommandBuffer*` (wraps native `VkCommandBuffer`) | `gpu::CommandBuffer*` (wraps native `MTL::CommandBuffer*`) |
| Staging memory          | VMA buffer, `VMA_MEMORY_USAGE_GPU_TO_CPU`         | `MTL::Buffer`, `StorageModeShared`                 |
| Copy command            | `vkCmdCopyImageToBuffer` (TRANSFER_SRC)           | blit `copyFromTexture:toBuffer:`                   |
| Row layout              | tightly packed (`bufferRowLength = 0`)            | `bytesPerRow` may need 256-byte alignment → de-pad |
| Host access             | mapped staging pointer + `memcpy`                 | `buffer->contents()` + `memcpy`                    |

---

## 4. Cross-cutting conventions (apply to every step)

1. **Namespace boundary.** All engine changes live in `bg2e::gpu` (and `gpu::vk` / `gpu::metal`).
   The only files touched outside that namespace are the new example and its CMake.
2. **Compile-safe extension pattern.** New methods on abstract bases (`gpu::Image`) are added
   as **non-pure virtual with a default that throws**, so the base step compiles with both
   backends inheriting the default; each backend step overrides it.
3. **Metal isolation.** All Metal code stays under `BG2E_IS_MAC`; all Metal CMake stays under
   `if(APPLE)`. Linux/Windows compile the Metal sources as guarded stubs and never invoke
   Metal tools.
4. **Atomicity.** Each step is the smallest useful unit and leaves the project **compiling**.
   Compilation/verification is performed by the developer, not by this plan.
5. **No CMake edits to existing targets.** Auto-glob already picks up new `.hpp/.cpp` under
   `lib/`. The only new CMake file is `examples/gpu/06_offscreen_triangle/CMakeLists.txt`.
6. **No premature abstraction.** No `gpu::Buffer` type; staging buffers are inline per backend.

---

## 5. Ordered list of steps

| #   | File | Title | Type |
|-----|------|-------|------|
| 001 | `001-surface-refactor.md` | Intermediate `vk::Surface` / `metal::Surface` (virtual base, common depth/device code) | Refactor |
| 002 | `002-vulkan-immediate-submit.md` | Base `gpu::Device::immediateSubmit` (abstract closure, default throws) + Vulkan override | Base API + Backend |
| 003 | `003-metal-immediate-submit.md` | Metal `gpu::Device::immediateSubmit` override (commit + waitUntilCompleted) | Backend |
| 004 | `004-base-image-readpixels.md` | Base `gpu::Image::readPixelsRGBA8` (non-pure virtual, default throws) | Base API |
| 005 | `005-vulkan-image-readpixels.md` | `vk::Image::readPixelsRGBA8` (VMA staging + copy) | Backend |
| 006 | `006-metal-image-readpixels.md` | `metal::Image::readPixelsRGBA8` (blit + shared buffer) | Backend |
| 007 | `007-example-06-offscreen-triangle.md` | `examples/gpu/06_offscreen_triangle` CLI: render + read-back + save | Example |

---

## 6. Dependencies between steps

```
001                       (independent refactor; can land first or last, but recommended first)
002 ──► 005               (vulkan read-back needs vulkan immediate submit)
003 ──► 006               (metal read-back needs metal immediate submit)
004 ──► 005, 006          (base read-back API before backend overrides)
005, 006 ──► 007          (example needs read-back on the active backend)
```

Notes:
- 001 is independent of 002–007 and is recommended first so later steps build on the
  refactored surfaces, but it could also land last without affecting the others.
- 002/004 (Vulkan path) and 003 (Metal path) are independent of each other; the example (007)
  only requires the read-back of whichever backend it is run against, but both must **compile**.

---

## 7. Expected state at the end of each phase

| Phase | Steps | Project state at the end |
|-------|-------|--------------------------|
| Surface refactor | 001 | `vk::Surface`/`metal::Surface` host common depth/device code; all examples (03/04/05) build & behave identically. |
| Immediate submit | 002–003 | `gpu::Device::immediateSubmit(closure over gpu::CommandBuffer*)` runs a synchronous, backend-agnostic command buffer on both backends; nothing uses it yet. Builds on all platforms. |
| Read-back API | 004 | `gpu::Image::readPixelsRGBA8` exists (default throws). Builds. |
| Vulkan read-back | 005 | `vk::Image::readPixelsRGBA8` copies an RGBA8 image to a `std::vector<uint8_t>`. Builds & works on Vulkan. |
| Metal read-back | 006 | `metal::Image::readPixelsRGBA8` does the same on macOS; guarded stub elsewhere. Builds. |
| Offscreen example | 007 | `gpu_offscreen_triangle` renders the triangle to an offscreen image and writes `out.jpg` to the working directory on both backends. |

---

## 8. General validation criteria

For **every** step:
- Engine library `bg2e` and all existing examples (especially `examples/gpu/03`, `04`, `05`)
  compile.
- On macOS, both Vulkan and Metal backends compile; on Linux/Windows, Metal code compiles as
  guarded stubs and no Metal tool is invoked.
- No symbol or behaviour change outside `bg2e::gpu` except the new example + its CMake.

For the **functional milestone** (after 007):
- Running `gpu_offscreen_triangle` (Vulkan on Linux/Windows; Metal *or* Vulkan on macOS)
  produces an `out.jpg` in the working directory showing the `vertex_id` triangle over the
  clear color, with no validation errors.
- The saved image dimensions match the offscreen surface size (e.g. 800×600).

Verification (compiling and running) is performed by the developer; this plan does not
compile code.

---

## 9. Open design decisions (resolved here)

- **Where does `immediateSubmit` live?** → A **public, backend-agnostic** method on the
  abstract `gpu::Device`, taking a closure over the abstract `gpu::CommandBuffer`. Rationale:
  it is a general-purpose primitive that high-level, backend-independent code will reuse
  repeatedly (uploads, one-shot transitions, mip generation, etc.), not just the read-back.
  The read-back copy itself uses a native command (`vkCmdCopyImageToBuffer`, Metal blit) that
  is deliberately **not** on the abstract `gpu::CommandBuffer`; the backend `Image` records the
  abstract transitions through the closure's `cmd`, then down-casts that same `cmd` to its
  native handle to emit the single copy command. So `immediateSubmit` stays generic while the
  low-level read-back reaches the native handle only where it must.
- **Where does read-back live?** → On `gpu::Image` (`readPixelsRGBA8`), overridden per backend,
  mirroring the stable `render::vulkan::Image::readPixelsRGBA8`. The `Image` already stores its
  backend `Device`, so no extra device argument is needed.
- **How is the read-back synchronized?** → The example calls `device->waitIdle()` after the
  render submit (offscreen submit has no fence), and `readPixelsRGBA8` is itself fully
  synchronous via `immediateSubmit`.
- **Surface refactor shape?** → Virtual inheritance of `gpu::Surface`, inserting
  `vk::Surface`/`metal::Surface` as backend-common bases while preserving the typed factory.
  Alternative (flatten markers, return `gpu::Surface`) documented but not recommended.
- **Which formats?** → Only `R8G8B8A8_UNORM` read-back is implemented (offscreen default).
  Other formats throw a clear "unsupported format" error.
