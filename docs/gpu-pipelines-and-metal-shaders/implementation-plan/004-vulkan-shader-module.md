# Step 004 — Vulkan `vk::ShaderModule` (`VkShaderModule`)

## Objective

Implement the Vulkan shader module: load a `.spv` file and create a `VkShaderModule`, and override
`vk::Device::createShaderModule` to return it.

## Context

`gpu::ShaderModule` and the `Device` factory hook exist from step 003. `vk::Device::handle()`
exposes `VkDevice`. SPIR-V binaries already exist under `shaders/` and (for the example) under the
example's compiled output.

## Expected prior state

- Step 003 done. `gpu::Device::createShaderModule` throws by default.

## Files expected to review / modify

- Create: `lib/include/bg2e/gpu/vk/ShaderModule.hpp` — `vk::ShaderModule : gpu::ShaderModule`.
- Create: `lib/src/bg2e/gpu/vk/ShaderModule.cpp`.
- Modify: `lib/include/bg2e/gpu/vk/Device.hpp` + `lib/src/bg2e/gpu/vk/Device.cpp` — override
  `createShaderModule`.
- Review: `lib/src/bg2e/gpu/vk/Info.hpp` and `common.hpp` for helper/include conventions.

## Proposed design

- `vk::ShaderModule` holds `VkDevice` + `VkShaderModule` + `ShaderStage` + entry point.
- Construction: read `description.filePath` (binary), `vkCreateShaderModule` with the SPIR-V code.
  Store the entry point string (defaults to `"main"`).
- `cleanup()` → `vkDestroyShaderModule`; destructor calls `cleanup()`.
- Expose `handle()` → `VkShaderModule` and `entryPoint()` for the pipeline step to consume.
- `vk::Device::createShaderModule` constructs and returns `std::make_unique<vk::ShaderModule>(...)`.

## Required changes (no code in this plan)

- New `vk/ShaderModule.{hpp,cpp}` (auto-globbed; no CMake change).
- Override in `vk::Device`.

## Compilation criteria

- Engine builds on all platforms (Vulkan path is cross-platform).
- No example uses it yet → no behaviour change.

## Validation criteria

- Calling `vkDevice->createShaderModule({ "<some>.spv", "main", Vertex })` returns a valid module
  (exercised later by the example; here it must at least compile and link).

## Risks / points to check

- SPIR-V must be read as binary, 4-byte aligned, size in bytes; `pCode` is `uint32_t*`.
- File-not-found handling: throw a clear error (the example resolves the correct path).
- Lifetime: module must outlive pipeline creation (the example owns it).

## What must NOT be done in this step

- No Metal implementation (step 005).
- No pipeline creation; do not wire any example.
