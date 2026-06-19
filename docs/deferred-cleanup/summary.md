# Deferred Cleanup API for bg2e::gpu

## Problem Statement

The production rendering API (`bg2e::render::Engine`) supports deferred execution of closures tied to frame indices, primarily used for safe resource destruction after the GPU finishes using them (e.g., BLAS rebuilding in `scene::Drawable`). The experimental `bg2e::gpu` API lacks this capability. A new deferred cleanup mechanism is needed in `gpu::CleanupManager` that:

1. Schedules resource destruction closures tied to the frame counter
2. Executes them only after `inFlightFrames` frames have elapsed
3. Uses a naming convention that conveys "deferred cleanup" rather than generic "deferred execution"

Additionally, `gpu::Surface` needs:
- A `frameCounter` (monotonically increasing `uint64_t`) to index deferred closures
- An `inFlightFrames()` method (2 for window surfaces, 1 for offscreen) distinct from `imageCount()`

## Architecture

```
                  ┌─────────────────────────────────────────────┐
                  │              gpu::Surface                    │
                  │                                             │
                  │  imageCount()     → swapchain images (N)    │
                  │  inFlightFrames() → concurrent frames (2/1) │
                  │  frameCounter()   → total frames (uint64)   │
                  │                                             │
                  │  endFrame() {                               │
                  │      ...                                    │
                  │      ++_frameCounter;  // auto-increment    │
                  │  }                                          │
                  └──────────────┬──────────────────────────────┘
                                 │
                                 │ pointer (non-owning)
                                 ▼
                  ┌─────────────────────────────────────────────┐
                  │          gpu::CleanupManager                 │
                  │                                             │
                  │  Surface* _surface;                         │
                  │                                             │
                  │  defer(closure):                            │
                  │    targetFrame = _surface->frameCounter()   │
                  │               + _surface->inFlightFrames()  │
                  │    _deferredCleanups.push_back({target, c}) │
                  │                                             │
                  │  flushDeferred():                           │
                  │    for each where targetFrame <= counter:   │
                  │      closure()                              │
                  │      remove from vector                     │
                  │                                             │
                  │  flushAllDeferred():                        │
                  │    execute all pending closures             │
                  │    clear vector                             │
                  └─────────────────────────────────────────────┘

Render Loop:
  ┌──────────────────────────────────────────────────────────┐
  │  frame = surface->beginFrame()   // waits fence          │
  │  ... render ...                                          │
  │  surface->endFrame(frame)        // ++_frameCounter      │
  │  cleanupManager.flushDeferred()  // run expired closures │
  └──────────────────────────────────────────────────────────┘

Application Exit:
  ┌──────────────────────────────────────────────────────────┐
  │  device->waitIdle()                                      │
  │  cleanupManager.flushAllDeferred()  // run everything    │
  │  cleanup.flush()                                           │
  └──────────────────────────────────────────────────────────┘
```

## Files to Modify/Create

| # | File | Action | Description |
|---|------|--------|-------------|
| 1 | `lib/include/bg2e/gpu/Surface.hpp` | Modify | Add `inFlightFrames()`, `frameCounter()`, `_frameCounter` |
| 2 | `lib/include/bg2e/gpu/vk/WindowSurface.hpp` | Modify | Add `inFlightFrames()` override |
| 3 | `lib/src/bg2e/gpu/vk/WindowSurface.cpp` | Modify | Implement `inFlightFrames()=2`, increment counter in `endFrame()` |
| 4 | `lib/include/bg2e/gpu/vk/OffscreenSurface.hpp` | Modify | Add `inFlightFrames()` override |
| 5 | `lib/src/bg2e/gpu/vk/OffscreenSurface.cpp` | Modify | Implement `inFlightFrames()=1` |
| 6 | `lib/include/bg2e/gpu/metal/WindowSurface.hpp` | Modify | Add `inFlightFrames()` override |
| 7 | `lib/src/bg2e/gpu/metal/WindowSurface.cpp` | Modify | Implement `inFlightFrames()=2`, increment counter in `endFrame()` |
| 8 | `lib/include/bg2e/gpu/metal/OffscreenSurface.hpp` | Modify | Add `inFlightFrames()` override |
| 9 | `lib/src/bg2e/gpu/metal/OffscreenSurface.cpp` | Modify | Implement `inFlightFrames()=1` |
| 10 | `lib/include/bg2e/gpu/CleanupManager.hpp` | Modify | Constructor with `Surface*`, `defer()`, `flushDeferred()`, `flushAllDeferred()` |
| 11 | `lib/src/bg2e/gpu/CleanupManager.cpp` | Modify | Implement deferred cleanup methods |
| 12 | `lib/include/bg2e/gpu/FrameResourceRing.hpp` | Modify | `imageCount()` → `inFlightFrames()` |
| 13 | `examples/gpu/12_deferred_cleanup/CMakeLists.txt` | Create | Build config for validation example |
| 14 | `examples/gpu/12_deferred_cleanup/shaders/cube.vert.glsl` | Create | Vertex shader (copy from 07) |
| 15 | `examples/gpu/12_deferred_cleanup/shaders/cube.frag.glsl` | Create | Fragment shader (copy from 07) |
| 16 | `examples/gpu/12_deferred_cleanup/shaders/cube.vert.metal` | Create | Metal vertex shader (copy from 07) |
| 17 | `examples/gpu/12_deferred_cleanup/shaders/cube.frag.metal` | Create | Metal fragment shader (copy from 07) |
| 18 | `examples/gpu/12_deferred_cleanup/src/main.cpp` | Create | Validation example |
| 19 | `examples/CMakeLists.txt` | Modify | Add `add_subdirectory(gpu/12_deferred_cleanup)` |

## Step-by-Step Plan

- [Step 01: Surface frame counter and inFlightFrames](step-01-surface-frame-counter.md)
- [Step 02: vk::WindowSurface implementation](step-02-vk-window-surface.md)
- [Step 03: vk::OffscreenSurface implementation](step-03-vk-offscreen-surface.md)
- [Step 04: metal::WindowSurface implementation](step-04-metal-window-surface.md)
- [Step 05: metal::OffscreenSurface implementation](step-05-metal-offscreen-surface.md)
- [Step 06: FrameResourceRing update](step-06-frame-resource-ring.md)
- [Step 07: CleanupManager deferred API](step-07-cleanup-manager.md)
- [Step 08: Validation example](step-08-example.md)

## Thread Safety Notes

- `frameCounter` is incremented in `endFrame()`, which is called from the main thread only
- `flushDeferred()` is called from the main thread after the fence, so no concurrent access
- `CleanupManager` is not thread-safe by design (single-threaded render loop)
- No mutexes needed; all access is sequential within the render loop

## Key Design Decisions

1. **`inFlightFrames()` is virtual, `frameCounter()` is not** — each surface type defines its concurrency level, but the counter logic is shared
2. **`_frameCounter` is protected** — subclasses in concrete backends can access it for incrementing
3. **`CleanupManager` holds a non-owning `Surface*`** — the surface is owned by the caller (typically a `shared_ptr` in the example's scope)
4. **`imageCount()` is preserved** — it represents swapchain image count, which is semantically distinct from in-flight frames
5. **`incrementFrameCounter()` is not a public method** — the increment happens automatically inside `endFrame()` in each concrete surface
