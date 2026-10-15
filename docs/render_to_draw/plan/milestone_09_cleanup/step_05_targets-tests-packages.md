# M09-05 — Retire legacy shaders/examples and repair tests/distribution

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M09-01–M09-04.

## Scope and deliverable

- `CMakeLists.txt`
- `lib/CMakeLists.txt`
- `apps/CMakeLists.txt`
- `examples/CMakeLists.txt`
- `tests/CMakeLists.txt`
- `tests/test_02_uv_surface/src/main.cpp`
- `tests/test_03_integrated_baker_context/src/main.cpp`
- `cmake/`
- `scripts/`
- `shaders/src/glsl/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [CMakeLists.txt](../../../../CMakeLists.txt), [lib/CMakeLists.txt](../../../../lib/CMakeLists.txt), [apps/CMakeLists.txt](../../../../apps/CMakeLists.txt), [examples/CMakeLists.txt](../../../../examples/CMakeLists.txt), [tests/CMakeLists.txt](../../../../tests/CMakeLists.txt), [tests/test_02_uv_surface/src/main.cpp](../../../../tests/test_02_uv_surface/src/main.cpp), [tests/test_03_integrated_baker_context/src/main.cpp](../../../../tests/test_03_integrated_baker_context/src/main.cpp), [cmake/](../../../../cmake), [scripts/](../../../../scripts), [shaders/src/glsl/](../../../../shaders/src/glsl).

## Implementation work

1. Remove old application/example/render shader targets that require the deleted API, or migrate a specifically useful demonstration/check to draw/bake. No old application source support remains.
2. Adapt existing UV-surface/integrated-context/atlas executable checks and any supported tests to final public APIs; do not leave failing unbuilt targets to hide dependencies.
3. Update engine shader library install/bundle paths, optional FSR/native backend assets, packaging scripts and standalone-project templates to new headers/default entry points.
4. Delete legacy-only shaders/helpers after checking shared use; retain/relocate shared GLSL/Metal libraries into paired pipeline layout without output collisions. Preserve Vulkan SDK/Metal deployment still needed by GPU.

## Verification and exit conditions

- Configure/build/install/package checks, when authorized, contain no missing/dead render targets, headers or resources.
- A generated standalone project and installed new products load final shader libraries; adapted tests/examples do not include legacy APIs.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M10 build/tutorial/distribution pages must match final target/source/runtime paths, including retained GPU backend libraries.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-16 reviewed baseline

Preserve the real draw_deferred_renderer target and later accepted products; disposition each manual M01 probe explicitly rather than claiming an automatic test suite. Keep paired shaders/src/draw pipeline paths, gpu::vk::FidelityFXBridge optional SDK linkage and recursive standalone copies. Remove legacy-only shaders/targets only after the per-file manifest confirms no surviving consumer. See [M01 review](../milestone_01_deferred_renderer/review.md).
