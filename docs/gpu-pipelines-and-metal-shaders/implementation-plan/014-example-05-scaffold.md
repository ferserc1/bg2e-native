# Step 014 — Example `05_simple_triangle` scaffold (dual shader sources + CMake)

## Objective

Create `examples/gpu/05_simple_triangle` by **copying** `04_clear_loop`, add the two backend shader
source sets (GLSL for Vulkan, MSL for Metal) plus the CMake wiring to compile/bundle both. At the end
of this step the example still runs as a **clear loop** (no pipeline yet) but ships its shaders.

## Context

`02_compute_shader` is the reference for per-example shaders. The example must keep two shader sets
(no shared source): Vulkan GLSL→SPIR-V, Metal MSL→`.metallib` (macOS only). CMake helpers exist:
`compile_shaders` (GLSL) and `compile_metal_shaders` (step 002, macOS-only no-op elsewhere).

## Expected prior state

- Steps 001–013 done. `compile_metal_shaders` exists; pipeline + `bindPipeline` + `draw` available.

## Files expected to review / modify

- Create dir: `examples/gpu/05_simple_triangle/`.
- Create: `examples/gpu/05_simple_triangle/src/main.cpp` — copied verbatim from `04_clear_loop`
  (only window title changed). No pipeline use yet.
- Create: `examples/gpu/05_simple_triangle/CMakeLists.txt`.
- Create GLSL: `examples/gpu/05_simple_triangle/shaders/glsl/triangle.vert.glsl`,
  `.../triangle.frag.glsl` (vertex_id triangle; positions+colors hardcoded by `gl_VertexIndex`).
- Create MSL: `examples/gpu/05_simple_triangle/shaders/metal/triangle.metal` (a `vertex` and a
  `fragment` function generating the same triangle from `[[vertex_id]]`).
- Modify: `examples/CMakeLists.txt` — `add_subdirectory(gpu/05_simple_triangle)`.
- Review: `bundle_app_sdl` (used by 04) and how it bundles per-app shaders to `shaders/<target>`.

## Proposed design

- **Source layout decision (per-backend folders):**
  ```
  examples/gpu/05_simple_triangle/shaders/glsl/triangle.vert.glsl
                                          /glsl/triangle.frag.glsl
                                          /metal/triangle.metal
  ```
- **CMake wiring:** in the example `CMakeLists.txt`:
  - Use `bundle_app_sdl(TARGET_NAME gpu_simple_triangle SHADERS_SRC <glsl dir>)` to compile GLSL→SPIR-V
    and bundle it (mirrors 02/04). The example loads SPIR-V from the bundled `shaders/<target>` path.
  - Call `compile_metal_shaders(${APP_TARGET_NAME} <metal dir> <metal out dir>)` (macOS-only no-op
    elsewhere) and bundle the resulting `.metallib` next to the SPIR-V (e.g. `shaders/<target>/metal`).
  - Decision point: confirm whether `bundle_app_sdl` needs a new optional `METAL_SHADERS_SRC` keyword
    (cleanest) **or** the example calls `compile_metal_shaders` + `bundle_resources` directly. Either
    is acceptable; prefer extending `bundle_app_sdl` with an optional `METAL_SHADERS_SRC` so the
    pattern is reusable. Implement that extension here if chosen (it is build tooling, allowed).
- **Shaders (content authored in this step, but they are example assets, not engine shaders):**
  - GLSL `triangle.vert`: emit 3 clip-space positions + 3 colors selected by `gl_VertexIndex`.
  - GLSL `triangle.frag`: output the interpolated color.
  - MSL `triangle.metal`: `vertex` function using `[[vertex_id]]` returning position+color; `fragment`
    returning the color. Use distinct, documented entry-point names (e.g. `triangle_vertex`,
    `triangle_fragment`) — the example will pass these to `createShaderModule` on Metal.
- `main.cpp` stays a clear loop in this step (pipeline added in 015) to keep the step atomic and
  buildable.

## Required changes (no code logic in engine; example + CMake only)

- New example dir, `main.cpp` (copy), CMake, GLSL + MSL sources.
- `examples/CMakeLists.txt` add_subdirectory.
- Optional `bundle_app_sdl` `METAL_SHADERS_SRC` extension (build tooling).

## Compilation criteria

- Configure + build on all platforms; on macOS both SPIR-V and `.metallib` are produced and bundled;
  on Linux/Windows only SPIR-V is produced (no Metal tool invoked).
- `gpu_simple_triangle` builds and runs as a clear loop.

## Validation criteria

- Bundled output contains `triangle.vert.spv` + `triangle.frag.spv` (all platforms) and
  `triangle.metallib` (macOS).
- Running the example shows the animated clear (unchanged from 04) — confirming wiring without
  regressions before adding the pipeline.

## Risks / points to check

- Shader file paths at runtime: the example must resolve the bundled shader directory the same way
  other examples do (check how `02_compute_shader`/`04` locate `shaders/<target>`).
- MSL entry-point names must match what step 015 passes to `createShaderModule`.
- Keep GLSL `lib/` includes out of the example shaders (they are standalone) to avoid include-path issues.

## What must NOT be done in this step

- Do **not** create or bind a pipeline yet (step 015).
- Do **not** add engine shaders. Do not introduce a shared GLSL/MSL source.
