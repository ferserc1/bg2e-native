# Step 002 — Metal shader compile tools (macOS only) + `shaders/metal`

## Objective

Add CMake infrastructure to compile MSL (`.metal`) sources into `.metallib`, **only on macOS**,
isolated so Linux/Windows never search for or invoke Metal tooling. Prepare `shaders/metal` as the
Metal output location. **No Metal shader is added yet** — only the reusable tooling.

## Context

No Metal compilation exists. The reference for per-target shader compilation is `compile_shaders`
in `cmake/utils.cmake` and its use by `bundle_app`/`bundle_app_sdl`. Metal compilation uses the
Xcode toolchain: `xcrun -sdk macosx metal -c <file>.metal -o <file>.air` then
`xcrun -sdk macosx metallib <air files...> -o <out>.metallib`. The engine already links the Metal
framework and vendors `metal-cpp`.

## Expected prior state

- Step 001 done: GLSL under `shaders/src/glsl/`, `shaders/src/metal/` exists (empty).
- Engine + examples build on all platforms.

## Files expected to review / modify

- Modify: `cmake/utils.cmake` — add `compile_metal_shaders(TARGET, SRC_PATH, DST_PATH)` (and/or a
  `build_metal_shaders` variant mirroring `compile_shaders`).
- Review: `bundle_app` / `bundle_app_sdl` to decide where the **example** Metal hook will attach
  (the actual example wiring is done in step 014; here only the reusable function is added, plus an
  optional `METAL_SHADERS_SRC` keyword parsed but acceptable to be unused until 014).
- Review: `lib/CMakeLists.txt` — reserve `shaders/metal` engine output dir (created empty; no engine
  MSL compiled yet). Decide whether to export a `BG2E_METAL_SHADER_DIR` variable now (recommended,
  even if unused).

## Proposed design

- New function `compile_metal_shaders(TARGET_NAME SRC_PATH DST_PATH)`:
  - Wrapped entirely in `if(APPLE) ... endif()`. On non-Apple it is a no-op (define an empty stub so
    callers can call it unconditionally, mirroring `copy_vulkan_resources`).
  - `file(GLOB MSL_SRC "${SRC_PATH}/*.metal")`.
  - For each `.metal`: `add_custom_command` → `xcrun -sdk macosx metal -c <src> -o <air>`.
  - Final `add_custom_command` → `xcrun -sdk macosx metallib <all .air> -o <DST>/<name>.metallib`.
    - Decision point: **one metallib per source** vs **one combined metallib**. Recommend **one
      metallib per `.metal` source** for the first iteration (simplest mapping to `MTL::Library`);
      document the combined-library option for later. The example (step 014) will follow whichever
      is chosen here.
  - Create a `${TARGET}_metal_shaders` custom target and `add_dependencies(${TARGET} ...)`, mirroring
    `compile_shaders`.
- Engine output dir: `shaders/metal` under `${PRODUCT_DIR}` reserved; **do not** invoke
  `compile_metal_shaders` for the engine yet (no engine MSL exists). Optionally `make_directory` it
  and export `BG2E_METAL_SHADER_DIR`.

## Required changes (no code in this plan)

- **CMake:** add `compile_metal_shaders` (+ non-Apple no-op stub) to `cmake/utils.cmake`.
- **CMake:** optionally export `BG2E_METAL_SHADER_DIR = ${PRODUCT_DIR}/shaders/metal` from `lib/CMakeLists.txt`.
- No C++ changes. No shader sources added.

## Compilation criteria

- Configure + build succeeds on macOS (Xcode), Linux (Ninja), Windows (VS2022).
- On Linux/Windows, no `xcrun`/`metal`/`metallib` command is referenced or executed.
- Engine + examples unchanged at runtime.

## Validation criteria

- `compile_metal_shaders` exists and is a safe no-op off-Apple (can be called unconditionally).
- On macOS, calling it on a folder with a sample `.metal` would produce a `.metallib` (validated
  later by the example; not exercised here since no MSL is added).

## Risks / points to check

- `xcrun -sdk macosx metal` requires full Xcode (not just Command Line Tools). Document this
  prerequisite; the project already mandates the Xcode generator on macOS.
- Metal language/target version flags (`-std=metal3.0`, deployment target). Pick a conservative
  default and document it.
- Ensure the non-Apple stub has the same arity so example CMake can call it unconditionally.

## What must NOT be done in this step

- Do **not** add any definitive `.metal` source (engine or example). Add a simple `.metal` source in the engine code (shaders/src/metal/compile_test.metal) with the unique purpose to test the metal build system.
- Do **not** wire the example yet (step 014).
- Compile **only** the `compile_test.metal` Metal shader.
