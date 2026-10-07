# Handoff for connecting the existing xatlas dependency

Next implementation step: [../02_uv_atlas/step-01_xatlas_dependency.md](../02_uv_atlas/step-01_xatlas_dependency.md). Complete this file **after** finishing integrated step 08; this template records no UV-atlas implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Connect the existing xatlas source to the engine build as specified in ../02_uv_atlas/step-01_xatlas_dependency.md; do not vendor another copy.

## Next-step instructions

Use the xatlas source and MIT `LICENSE.txt` already present in `lib/third_party/xatlas/`. Add its source and private include directory to the engine's CMake dependency lists without changing unrelated build logic. Keep xatlas behind geo-facing types; public headers must not expose its API.

## Next-step acceptance gate

The engine links xatlas and otherwise behaves as before. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Confirm `lib/third_party/xatlas/src/xatlas.cpp`, `src/xatlas.h` and `LICENSE.txt` are present and that the license is MIT. Do not fetch, replace or duplicate the supplied dependency. Record source provenance or an exact revision only if it can be verified from the supplied files; do not invent one.
- In `lib/cmake/deps.cmake`, add `${THIRD_PARTY_PATH}/xatlas/src` to `BG2E_THIRD_PARTY_INCLUDE_PATH` and `${THIRD_PARTY_PATH}/xatlas/src/xatlas.cpp` to `BG2E_THIRD_PARTY_SRC`. `lib/CMakeLists.txt` already consumes those variables for the bg2e target. Keep the include path private to the engine target; no public `bg2e` header may include `xatlas.h`.
- Confirm supported Linux/macOS/Windows compiler settings and C++20 mode remain unchanged. Only this step connects the dependency; actual modifier behavior follows in step 02.
