# M08-04 — Build apps/draw/lightmap_generator and configuration flow

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M08-01–M08-03.

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

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M08-05 validates full CLI parity. M09 determines final installed executable naming from surviving targets without restoring old API support.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
