# M08-04 — Build apps/draw/lightmap_generator and configuration flow

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M08-01–M08-03.

## Scope and deliverable

- `apps/lightmap_generator/src/CommandLine.hpp`
- `apps/lightmap_generator/src/CommandLine.cpp`
- `apps/lightmap_generator/src/main.cpp`
- `apps/draw/lightmap_generator/`
- `apps/CMakeLists.txt`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/lightmap_generator/src/CommandLine.hpp](../../../../apps/lightmap_generator/src/CommandLine.hpp), [apps/lightmap_generator/src/CommandLine.cpp](../../../../apps/lightmap_generator/src/CommandLine.cpp), [apps/lightmap_generator/src/main.cpp](../../../../apps/lightmap_generator/src/main.cpp), [apps/CMakeLists.txt](../../../../apps/CMakeLists.txt).

## Implementation work

1. Inventory current CLI arguments/configuration/jobs/output codes and create the new target with equivalent useful behavior over draw/bake.
2. Use experimental OffscreenApplication/context initialization and standalone scheduler; do not instantiate a camera renderer just to access effect/material/environment resources.
3. Support explicit backend and configuration-file import for active bake/effect options using the settings codec where compatible. Render-only unavailable properties follow declared diagnostics; bake job settings own their values, not the stateless preferences facade.
4. Preserve output/preflight/overwrite/UV2 controls and packaged assets/shaders. Keep the original CLI working until cleanup and expose deterministic error/exit behavior.

## Verification and exit conditions

- Representative model/prefab/context batches run headlessly and produce expected maps/model copies at requested sample settings.
- Invalid args/config, unavailable RT, output errors and canceled/failed jobs have clear non-success status and clean shutdown.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M08-05 validates full CLI parity. M09 determines final installed executable naming from surviving targets without restoring old API support.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M03-10 reviewed implementation requirement

Import stable live effect settings through the codec at a stopped boundary with explicit required-capability checks. UI-only lightPose/document targets need a declared mapping or reject/skip diagnostic policy. Keep CLI dimensions/sample overrides last; no implicit preferences or RendererDeferred dependency for UV tracing.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.


## M04-07 reviewed implementation requirement

Deploy both paired shader trees, draw and bake, through the engine shader aggregate and RuntimeShaders installation. Check bake/uv_surface/surface.vert.metallib (Metal) and its SPIR-V peer in installed/package resources; test an incremental shader-only rebuild as well as a fresh build. Keep required bake support distinct from no-RT raster fallback; the interactive example's exception UI is not CLI exit/error policy.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).
