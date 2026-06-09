# Step 020 — Example `05_simple_triangle`: minimal compute validation

## Objective

Extend `05_simple_triangle` to **create and dispatch** a minimal compute pipeline once (or per frame),
proving `gpu::ComputePipeline` + `cmd->dispatch()` work on both backends. The compute result need
**not** be visible — it only must create and execute without errors.

## Context

Compute pipeline (016–018) and dispatch (019) exist. The example already ships GLSL+MSL for the
triangle; this step adds a compute shader per backend (GLSL `.comp` → SPIR-V, MSL kernel → metallib).

## Expected prior state

- Steps 015 (triangle renders) and 019 (dispatch) done.

## Files expected to review / modify

- Create GLSL: `examples/gpu/05_simple_triangle/shaders/glsl/noop.comp.glsl` (a trivial kernel, e.g.
  writes to a small storage buffer or just runs — keep it dependency-free; a bare `main` with a fixed
  local size is enough for validation).
- Add MSL: a `kernel` function in `triangle.metal` (or a new `compute.metal`), e.g. `noop_compute`.
- Modify: `examples/gpu/05_simple_triangle/src/main.cpp` — create compute shader module + compute
  pipeline; in the loop, before `beginRendering`, bind compute pipeline and `dispatch(1,1,1)`.
- Review: example CMake already compiles GLSL dir + metallib (step 014); the new `.comp`/kernel are
  picked up by the existing globs — confirm `compile_shaders` handles `.comp.glsl` (it does, via the
  generic branch) and that the kernel function is included in the metallib.

## Proposed design

- Minimal compute shader: no external resources required (a self-contained kernel). If a write target
  is desired for realism, document it but prefer the simplest dependency-free kernel for validation.
- In `main.cpp`:
  ```
  auto cs       = device->createShaderModule({ csPath, csEntry, ShaderStage::Compute });
  auto cLayout  = device->createPipelineLayout({});
  auto cPipe    = device->createComputePipeline({ cs.get(), cLayout.get() });
  ...
  // each frame, outside the render scope:
  cmd->begin();
  cmd->bindPipeline(cPipe.get());
  cmd->dispatch(1, 1, 1);
  // ... then the existing transition + beginRendering + triangle + present
  ```
- Keep `cs`, `cLayout`, `cPipe` alive for the loop; clean up before `device->cleanup()`.

Decision point — **dispatch placement:** record the dispatch **before** `transition`/`beginRendering`
(Vulkan: compute outside rendering scope; Metal: compute encoder must close before the render encoder
opens). Document this ordering in the example.

## Required changes (example + shaders only)

- New compute shaders (GLSL + MSL kernel), compute pipeline creation + dispatch in `main.cpp`.

## Compilation criteria

- Build green on all platforms; compute SPIR-V produced everywhere, kernel in metallib on macOS.

## Validation criteria

- Example still renders the triangle AND performs a compute dispatch each frame with no validation
  errors on either backend.

## Risks / points to check

- Encoder ordering on Metal (compute encoder must be ended before the render encoder opens).
- Vulkan: dispatch must be outside `vkCmdBeginRendering`/`vkCmdEndRendering`.
- Ensure the compute shader compiles under both glslang (`.comp`) and `metal` (kernel).

## What must NOT be done in this step

- No storage-buffer/resource-binding API (that is a later phase). If the kernel writes anything, wire
  it ad-hoc in the example, not via a new engine resource-binding abstraction.
