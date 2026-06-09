# Step 001 — Refactor shader tooling to `shaders/src/glsl/`

## Objective

Bifurcate the engine shader source tree so GLSL lives under `shaders/src/glsl/` and a sibling
`shaders/src/metal/` folder is reserved for MSL. The Vulkan SPIR-V output must keep landing in
`shaders/` exactly as today. **No new shader is added; no runtime behaviour changes.**

## Context

Today engine GLSL lives directly in `shaders/src/*.glsl` with shared includes in `shaders/src/lib/`.
`cmake/utils.cmake` (`build_shaders`, `compile_shaders`) and the standalone `shaders/build.sh` /
`shaders/build.bat` glob `${SRC}/*.glsl`. `lib/CMakeLists.txt` sets `SRC_SHADER_DIR = shaders/src`
and compiles to `${PRODUCT_DIR}/shaders`. glslang resolves `#include` relative to `shaders/src/lib/`,
so `lib/` must keep the same relative position to the GLSL files.

## Expected prior state

- Engine + examples compile (post step 0 / current `main`).
- `shaders/src/*.glsl` (+ `shaders/src/lib/`, `shaders/src/test/`) present; SPIR-V emitted to `shaders/`.

## Files expected to review / modify

- Review: `cmake/utils.cmake` (`build_shaders`, `compile_shaders`, `bundle_app`, `bundle_app_sdl`).
- Review: `lib/CMakeLists.txt` (`SRC_SHADER_DIR`, `compile_shaders`, `BG2E_SHADER_DIR`).
- Review: `shaders/build.sh`, `shaders/build.bat`.
- Move (git mv): `shaders/src/*.glsl` → `shaders/src/glsl/*.glsl`, including `shaders/src/lib/` →
  `shaders/src/glsl/lib/` and `shaders/src/test/` → `shaders/src/glsl/test/` (keep `lib/` relative to GLSL).
- Create: empty `shaders/src/metal/` (with a `.gitkeep` / `README.md` placeholder).
- Modify: `lib/CMakeLists.txt` → `SRC_SHADER_DIR = ${CMAKE_SOURCE_DIR}/shaders/src/glsl`.
- Verify: any other reference to `shaders/src` as a GLSL root (grep the repo).

## Proposed design

- Move the entire current GLSL tree under `shaders/src/glsl/` preserving internal structure
  (`glsl/`, `glsl/lib/`, `glsl/test/`). Because `lib/` moves together with the GLSL files, the
  `#include "lib/..."` relative paths stay valid.
- Point `lib/CMakeLists.txt`'s `SRC_SHADER_DIR` at the new `shaders/src/glsl`.
- Keep `DST_SHADER_DIR = ${PRODUCT_DIR}/shaders` unchanged → SPIR-V output location is identical.
- Update `shaders/build.sh` / `shaders/build.bat` default input to `shaders/src/glsl` (or document
  that the input dir argument must now be `shaders/src/glsl`).
- `compile_shaders` / `build_shaders` functions are unchanged in signature; only the *caller's*
  `SRC_PATH` changes. Per-example shader globbing (`02_compute_shader`) is unaffected (its sources
  stay under the example).

## Required changes (no code in this plan)

- **CMake:** edit `SRC_SHADER_DIR` in `lib/CMakeLists.txt`. Confirm `RT`-stage detection in
  `compile_shaders` still matches (it keys on file-name substrings, not paths).
- **Scripts:** update `shaders/build.sh` / `.bat` input directory references.
- **Filesystem:** `git mv` the GLSL tree; add `shaders/src/metal/` placeholder.

## Compilation criteria

- `cmake -S . -B build -G <platform generator>` configures without error.
- Build produces the same `shaders/*.spv` files as before in the same location.
- Engine + all examples link and run identically (clear loop unchanged).

## Validation criteria

- `shaders/` (SPIR-V) content is byte-for-byte equivalent set as before the move.
- No `.glsl` remain directly under `shaders/src/` (only under `shaders/src/glsl/`).
- `shaders/src/metal/` exists and is empty (placeholder only).

## Risks / points to check

- **Include path breakage:** if `lib/` is *not* moved together with the GLSL files, glslang
  `#include` resolution breaks. Move `lib/` into `shaders/src/glsl/lib/`.
- Hidden references to `shaders/src` elsewhere (apps, tooling, CI). Grep before moving.
- `.DS_Store` files under `shaders/src` — do not move/track them.

## What must NOT be done in this step

- Do **not** add any Metal shader or any new GLSL shader.
- Do **not** change SPIR-V output location or any C++.
- Do **not** add Metal compilation tooling (that is step 002).
