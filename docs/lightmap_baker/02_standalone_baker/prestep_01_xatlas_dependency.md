# Handoff for Introduce xatlas under MIT

Next implementation step: [../03_uv_atlas/step-01_xatlas_dependency.md](../03_uv_atlas/step-01_xatlas_dependency.md). Complete this file **after** finishing step 08; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in ../03_uv_atlas/step-01_xatlas_dependency.md.

## Next-step instructions

Vendor a pinned xatlas release with its MIT LICENSE and provenance. Add its source to the engine build only as necessary for compilation; do not alter unrelated CMake logic. Wrap the dependency behind geo-facing types so public headers do not expose xatlas. Record the single-atlas requirement and error handling.

## Next-step acceptance gate

The engine links xatlas and otherwise behaves as before. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Vendor the official [jpcy/xatlas](https://github.com/jpcy/xatlas) C++ source under `lib/third_party/xatlas/` with its [MIT license](https://github.com/jpcy/xatlas/blob/master/LICENSE), source URL and an exact revision recorded in a local provenance file. Do not use a GPL/LGPL fork or copy a transitive dependency without its notice.
- Compile xatlas source into the engine's existing library target with the smallest CMake change needed. Keep xatlas headers private to `lib/src/bg2e/geo`; no public `bg2e` header may include them. Only this step introduces the dependency; actual modifier behavior follows in step 02.
- Confirm supported Linux/macOS/Windows compiler settings and C++20 mode remain unchanged. Record the revision in the next prestep so later agents do not silently upgrade it.
