# 6.4 Verify scene save, standalone export and SDK interoperability

## Entry condition

[6.3 Publish the recipe and owner as one change](../step_6/substep_3.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

lib/include/bg2e/db/mesh_bg2.hpp, lib/src/bg2e/db/{scene_bg2,mesh_bg2}.cpp, tests/draw/{composer_io,resource_io,drawable_resources}.

## Implementation contract

Use the native scene save path for both component data and conventional baked model/assets, including Save As and relative references. Reuse exportDrawableModelBg2 for standalone extraction with mesh in owner-local coordinates, UVs/materials and normal asset dependencies. Reopen without regeneration. Exercise C++ recipe reload and a TypeScript reader that ignores the unregistered type. Make no TS-side implementation changes. If no TS consumer is available, distinguish a verified native artifact from an outstanding user-run compatibility check.

## Acceptance evidence

Use one split-wall scene and record scene/model/texture locations. Relocate them together and verify relative references and model-only loading. Check empty scene save and failed save/retry. Do not claim a TS pass based solely on inspecting JSON.

## User validation

Using an available fixture and consumer, open the same saved scene in C++ and TypeScript, then load the exported wall alone in an empty scene. Check the local pivot, finishes and UVs. If no UI producer exists yet, use a native fixture and repeat through Composer at the final gate.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [7.1 Build viewport rays in geobuild](../step_7/substep_1.md)
