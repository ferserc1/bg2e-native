# Step 005 — Metal `metal::ShaderModule` (`MTL::Library` / `MTL::Function`)

## Objective

Implement the Metal shader module: load a `.metallib`, obtain an `MTL::Function` by entry-point name,
and override `metal::Device::createShaderModule`. All Metal code guarded by `BG2E_IS_MAC`.

## Context

`gpu::ShaderModule` and the factory hook exist (003). `metal::Device::handle()` exposes
`MTL::Device*`. `metal-cpp` provides `MTL::Library`, `MTL::Function`. The metallib is produced by the
step-002 tooling (for the example, by step 014).

## Expected prior state

- Step 003 done; step 004 may or may not be done (independent). `createShaderModule` throws by default.

## Files expected to review / modify

- Create: `lib/include/bg2e/gpu/metal/ShaderModule.hpp` — `metal::ShaderModule : gpu::ShaderModule`,
  with `#if BG2E_IS_MAC` members and a non-Mac stub (mirroring `metal::CommandBuffer`).
- Create: `lib/src/bg2e/gpu/metal/ShaderModule.cpp` — `#if BG2E_IS_MAC` real impl + `#else` stubs.
- Modify: `lib/include/bg2e/gpu/metal/Device.hpp` + `lib/src/bg2e/gpu/metal/Device.cpp` — override
  `createShaderModule`.
- Review: `lib/include/bg2e/gpu/metal/common.hpp` for handle aliases / metal-cpp includes.

## Proposed design

- `metal::ShaderModule` holds `MTL::Library*` and `MTL::Function*` (+ `ShaderStage`).
- Construction (`BG2E_IS_MAC`):
  - Build an `NS::String` path → `device->newLibrary(NS::URL fileURL, &error)` (load `.metallib`).
  - `library->newFunction(NS::String entryPoint)` → `MTL::Function*`.
  - Retain/release per metal-cpp ownership rules; `cleanup()` releases function then library.
- Non-Mac: stub members compile to no-ops returning invalid (same pattern as `metal::CommandBuffer`'s `#else`).
- Expose `function()` → `MTL::Function*` for the pipeline step.
- `metal::Device::createShaderModule` returns `std::make_unique<metal::ShaderModule>(...)`.

Decision point — **one metallib holding many functions** vs **one metallib per stage**: must match
the choice in step 002. If "one metallib per source", each `ShaderModule` loads its own library and
fetches its single function. If "combined library", the example may load one library and create
several modules from it (consider a `createShaderModule(library, functionName)` overload later).

## Required changes (no code in this plan)

- New `metal/ShaderModule.{hpp,cpp}` (auto-globbed).
- Override in `metal::Device`.

## Compilation criteria

- macOS: compiles with real impl. Linux/Windows: compiles via `#else` stubs; no metal-cpp symbol use.
- No example uses it yet.

## Validation criteria

- On macOS, `metalDevice->createShaderModule({ "<x>.metallib", "vertexMain", Vertex })` returns a
  module wrapping a valid `MTL::Function` (exercised by the example later).

## Risks / points to check

- metal-cpp memory ownership: `newLibrary`/`newFunction` return retained objects → release in cleanup.
- `MTL::Function` entry-point names are the MSL function names (not "main"); the example must pass
  the actual function names.
- Error handling for missing/invalid metallib (capture `NS::Error`).

## What must NOT be done in this step

- No pipeline creation; no example wiring.
- Do not add engine MSL shaders.
