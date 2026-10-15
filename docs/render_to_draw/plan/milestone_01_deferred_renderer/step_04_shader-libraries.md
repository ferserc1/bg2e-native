# M01-04 — Paired pipeline shader folders and deployment

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M01-01–M01-03.

## Scope and deliverable

- `lib/include/bg2e/gpu/ShaderLib.hpp`
- `lib/src/bg2e/gpu/ShaderLib.cpp`
- `cmake/utils.cmake`
- `lib/CMakeLists.txt`
- `shaders/src/draw/lib/`
- `shaders/src/draw/gbuffer/`
- `shaders/src/draw/composite/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/gpu/ShaderLib.hpp](../../../../lib/include/bg2e/gpu/ShaderLib.hpp), [lib/src/bg2e/gpu/ShaderLib.cpp](../../../../lib/src/bg2e/gpu/ShaderLib.cpp), [cmake/utils.cmake](../../../../cmake/utils.cmake), [lib/CMakeLists.txt](../../../../lib/CMakeLists.txt).

## Implementation work

1. Create per-pipeline folders containing GLSL and Metal stage sources together, plus shared draw/lib includes. Keep old production paths separate during coexistence.
2. Inspect actual loader entry-point/file conventions and current compile helpers; configure library paths so each pipeline resolves its own stages without basename collisions.
3. Extend shader build/bundle/install rules only as needed for future authorized implementation. Include shared include dependency tracking and app/standalone runtime destinations.
4. Port shared math/PBR/color helpers as explicit backend shader code. Do not add automatic translation or implicit binding rules.

## Verification and exit conditions

- ShaderLib resolves corresponding vertex/fragment/compute stages on both backends from deployed paths, including an installed/bundled product.
- Production shaders remain loadable, and two pipelines with identically named stage files do not collide.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M01-08/M01-10/M01-11/M01-13 supply concrete libraries; M03/M04 follow the same layout; M09 removes only legacy libraries.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
