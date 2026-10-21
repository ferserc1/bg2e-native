# Tutorial 08: Deferred Cleanup

This tutorial adapts [GPU Deferred Cleanup](../gpu/12_deferred_cleanup.md)
to `bg2e::draw`. A rotating textured mesh alternates between a cube and a sphere
every five seconds of active scene time. Its previous vertex/index buffers are
retired through a deferred cleanup closure instead of being destroyed while
submitted frames may still reference them.

**Source:** [examples/draw/08_deferred_cleanup/src/main.cpp](../../../examples/draw/08_deferred_cleanup/src/main.cpp)

## What you will learn

- Why replacing a mesh requires retaining its previous GPU buffers
- How `CleanupManager::defer()` captures pending submission completion records
- How draw polls the engine's deferred cleanup manager after presentation
- How to capture resource ownership without capturing the scene delegate
- How to drain pending closures safely during resize and shutdown
- How to observe geometry replacement and actual release through UI counters
- How deferred retirement complements frame-slot uniform resource rings

## Prerequisites

- Completed [03_uniform_buffers](03_uniform_buffers.md)
- Familiarity with retained scene/UI composition from [01_window_ui](01_window_ui.md)
- A configured bg2e-native build environment with a valid Vulkan SDK
- GLSL and, on macOS, Metal shaders compiled and packaged by the target
- The Metal compiler toolchain on macOS

The example needs no ray tracing support or external textures. It uses the
draw/GPU path with procedural geometry and a 2x2 texture.

## Understanding deferred cleanup

A submitted draw command references GPU vertex/index buffers. Replacing the
CPU-side mesh does not mean the GPU has finished those draws. An old mesh must
stay alive until every captured submission that might reference it completes.

`CleanupManager::defer()` snapshots the device's pending completion records,
including tracked submissions across its queues. `flushDeferred()` polls these
records and executes eligible closures. Eligibility is based on actual
completion, not a fixed number of frames or elapsed seconds. `endFrame()` advances
bookkeeping; it does not certify GPU completion.

Future submissions and recorded-but-unsubmitted work are outside the snapshot.
The example therefore switches meshes during `update()`, before recording any
scene draw commands for the current frame. Every later scene draw uses the new
mesh. The old mesh is retained exclusively by the cleanup closure.

The five-second timer determines **when geometry changes**, not when retired
resources are safe to destroy. The UI includes a manual Switch geometry button,
pending/released mesh counts, and the existing background, pause, and refresh
controls. Creation and release are also logged to the console.

## Step-by-step code explanation

### 1. Reuse the draw application and textured mesh setup

The Application/MainLoop entry point, backend arguments, texture, pipeline
layout, and camera/model uniforms follow tutorial 03. The default backend is
Metal on macOS and Vulkan elsewhere; `--backend=vulkan` or `--backend=metal`
overrides it.

The shader pair is loaded from `shaders/draw_deferred_cleanup`. The unchanged
bindings are:

| Resource | Vulkan set/binding | Metal index | Stage |
|----------|--------------------|-------------|-------|
| Camera UBO | 0 / 0 | Buffer 2 | Vertex |
| Model UBO | 1 / 0 | Buffer 3 | Vertex |
| Texture | 2 / 0 | Texture 0 | Fragment |
| Sampler | 2 / 1 | Sampler 1 | Fragment |

The camera UBO is persistent and updated during resize after GPU work drains.
The model buffer and resource set use matching `FrameResourceRing` instances
indexed by `FrameContext::frameSlot`. Static resources use a delegate-local
CleanupManager for reverse-order teardown.

### 2. Create one wrapper for each mesh generation

The active mesh is a `shared_ptr<gpu::MeshPU>`. A fresh wrapper is built for every
geometry generation, so old buffers are never replaced inside a wrapper still
referenced by submitted draws:

```cpp
void createGeometry(bool sphere)
{
    std::unique_ptr<geo::MeshPU> meshData;
    if (sphere)
    {
        meshData.reset(geo::createSpherePU(0.8f, 24, 24));
    }
    else
    {
        meshData.reset(geo::createCubePU(1.0f, 1.0f, 1.0f));
    }
    _mesh = std::make_shared<gpu::MeshPU>();
    _mesh->setMeshData(*meshData);
    _mesh->build(_engine->device());
    std::cout << "Created " << (sphere ? "sphere" : "cube") << " mesh\n";
}
```

Initialization calls `createGeometry(false)` once. Both shapes use positions
and UVs, the same texture, and the same pipeline. Switching does not recreate
the shader modules, material descriptor set, or model resource rings.

### 3. Retire the old mesh before recording new draws

`update()` advances rotation time and a separate switch timer using scene delta
time. When the timer reaches five seconds, or a manual switch is requested, it
moves ownership of the current mesh into a deferred closure:

```cpp
const bool oldWasSphere = _controller->useSphere;
_engine->cleanupManager().defer(
    [oldMesh = std::move(_mesh), controller = _controller, oldWasSphere]()
    {
        oldMesh->cleanup();
        --controller->pendingMeshes;
        ++controller->releasedMeshes;
        std::cout << "Released retired " << (oldWasSphere ? "sphere" : "cube") << " mesh\n";
    });
++_controller->pendingMeshes;
++_controller->geometrySwitches;
_controller->useSphere = !oldWasSphere;
_controller->switchRequested = false;
_switchElapsed = 0.0f;
createGeometry(_controller->useSphere);
```

The closure captures shared ownership of the retired mesh and controller. It
does not capture `this` or a reference to a temporary variable. The controller
remains valid when the callback updates the counters, and explicit mesh cleanup
releases its GPU buffers at the correct time.

`defer()` queues the closure; it does not execute it immediately. The pending
counter is incremented before the frame loop can poll and execute the callback.
Rotation then updates the current slot's model UBO, and `render()` uses only the
new `_mesh`.

### 4. Let draw poll the engine manager

Retired meshes are scheduled on `_engine->cleanupManager()`, rather than the
local manager that owns static resources. Draw already integrates that manager
at the end of its frame path:

```cpp
surface->endFrame(frame.get());
_engine->cleanupManager().flushDeferred();
```

These lines belong to draw's render loop, not to the example's scene delegate.
The delegate leaves submission and presentation to draw. The poll runs after
presentation frames, including those that only compose UI over a paused scene.
Only closures whose captured dependencies have completed are executed; the
poll itself introduces no GPU wait.

The current mesh build implementation uploads vertex/index data synchronously
on both backends. Those uploads may complete older queued work before the next
poll, so pending mesh counts can return to zero very quickly. This example
preserves the source's upload path and demonstrates correct ownership and
retirement, rather than asynchronous mesh uploading or a guaranteed release
latency. Geometry switching adds no explicit `waitIdle()` call.

### 5. Render the current mesh into retained color and owned depth

The delegate owns a window-sized depth image because draw lends only a color
target. The visible pass clears background/depth, binds camera, current-slot
model, and material sets, and draws the active mesh:

```cpp
command.bindPipeline(_pipeline.get());
command.bindResourceSet(_pipeline.get(), 0, _cameraSet.get());
command.bindResourceSet(_pipeline.get(), 1, _modelSetRing.at(context.frameSlot));
command.bindResourceSet(_pipeline.get(), 2, _textureSet.get());
_mesh->draw(&command);
command.endRendering();
```

Draw handles retained scene copying, UI composition, command submission, and
presentation. Unchanged descriptor bindings are reused across geometry switches.

### 6. Keep controls and cleanup observable

The information window reports current geometry, total switches, pending mesh
releases, completed releases, scene updates, and UI frames. Switch geometry
sets a request flag and invalidates the scene; the actual replacement occurs
in the next active `update()` before any mesh draw is recorded. Multiple button
requests before that update are coalesced into one switch.

Pause freezes rotation and the automatic switch timer, retains the last scene,
and keeps UI presentation active. A manual switch, background change, or refresh
request made while paused stays pending until resume. Previously scheduled
cleanup can still complete while paused because draw continues polling its
engine manager during presentation.

### 7. Drain closures after draw's resize wait

Draw waits for outstanding GPU work before calling the delegate's `resize()`.
The delegate releases the old depth/pipeline and model rings, then drains
retired meshes:

```cpp
_engine->cleanupManager().flushAllDeferred();
```

`flushAllDeferred()` executes every queued callback unconditionally; it is safe
here because draw has already waited and frame production is on the same main
thread. The delegate does not repeat that wait. It then updates camera aspect
ratio and recreates depth, pipeline, and rings for the current surface extent
and in-flight frame count. The active mesh and switching timer remain unchanged.

Paused resize follows tutorial 01's retained image clear-color behavior.

### 8. Clean up the current mesh and persistent resources

Draw also drains GPU work before the delegate's shutdown callback. Cleanup
releases the visible pipeline, depth, and model rings, calls the engine manager's
`flushAllDeferred()`, then cleans the current mesh. The local static-resource
manager is flushed last, releasing sets before their buffers/images/layouts.

The retired mesh closures and active mesh have distinct ownership. The active
mesh is cleaned directly; retired meshes are cleaned only by their closures.
Draw retains ownership of the device, surface, and global engine manager.

## Shader code explanation

The four GLSL/Metal shader files preserve the source GPU example, with final
newline normalization only. The vertex shader projects positions using
`camera.projectionView * object.model` and passes UVs to the fragment shader:

```glsl
gl_Position = camera.projectionView * object.model * vec4(inPosition, 1.0);
fragUV = inTexCoord;
```

The fragment shader samples the procedural texture:

```glsl
outColor = texture(sampler2D(uTex, uSampler), fragUV);
```

Metal uses the corresponding camera/model buffer indices and texture/sampler
indices from the binding table. Deferred cleanup affects resource lifetime,
not the shader interface or texture sampling behavior.

## Key concepts

| Operation | Role in this example |
|-----------|----------------------|
| Local `push()` / `flush()` | Ordered teardown of persistent resources |
| Engine manager `defer()` | Retain the replaced mesh and snapshot pending submissions |
| Draw's `flushDeferred()` | Poll captured completions and execute ready callbacks |
| `flushAllDeferred()` | Drain callbacks after draw's coordinated resize/shutdown wait |
| Model resource rings | Isolate mutable per-frame uniform data |

A completion snapshot protects existing submitted uses. It cannot protect new
commands recorded or submitted afterward with the retired buffers. Retire first,
stop all future uses, then draw the replacement.

## Complete source

See [main.cpp](../../../examples/draw/08_deferred_cleanup/src/main.cpp) and the
[shader directory](../../../examples/draw/08_deferred_cleanup/shaders/).

## Building and running

The [CMakeLists.txt](../../../examples/draw/08_deferred_cleanup/CMakeLists.txt)
uses the GPU example's GLSL/Metal compilation and packaging calls with target
name `draw_deferred_cleanup`. ShaderLib loads `shaders/draw_deferred_cleanup`.

Use your configured build directory, replacing `build` as needed:

```sh
cmake --build build --target draw_deferred_cleanup --config Debug
```

On macOS:

```sh
./bin/macos/draw_deferred_cleanup.app/Contents/MacOS/draw_deferred_cleanup --backend=metal
./bin/macos/draw_deferred_cleanup.app/Contents/MacOS/draw_deferred_cleanup --backend=vulkan
```

On Linux, run `./bin/linux/draw_deferred_cleanup --backend=vulkan`.
On Windows, run `draw_deferred_cleanup.exe` under `bin/windows`.
The macOS target compiles Metal shaders even when launching Vulkan.

To inspect the result:

1. Wait five seconds of active scene time and observe cube/sphere alternation.
2. Compare switch/release counters and console creation/release messages.
3. Force several geometry changes with Switch geometry while active.
4. Pause, request a switch, and resume to apply it without advancing the paused timer.
5. Resize or close after a switch to exercise unconditional deferred draining.

## Next steps

- [Ray Tracing Pipeline](09_ray_tracing_pipeline.md) — Render a Cornell box with progressive accumulation and retained UI composition.
- [Draw tutorial index](index.md) — Follow future additions to the series.
- [GPU Deferred Cleanup](../gpu/12_deferred_cleanup.md) — Compare the low-level implementation.
- [CleanupManager](../../api/gpu/CleanupManager.md) — Ordered and completion-based cleanup.
- [Submission tracking and waitIdle](../../api/gpu/Submission_tracking_and_waitIdle.md) — Completion snapshots and coordinated waits.
- [FrameResourceRing](../../api/gpu/FrameResourceRing.md) — Reuse mutable resources per slot.
