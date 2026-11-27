# 5.4 Assemble the ordinary Drawable

## Entry condition

[5.3 Bake scene-aligned UVs into render vertices](../step_5/substep_3.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

lib/include/bg2e/draw/Drawable.hpp, geo mesh modifiers and ordinary Drawable construction examples.

## Implementation contract

Emit one submesh for each populated explicit group in creation order; place default first if populated. Persist group labels with submesh definitions, initialize conventional materials and expose the group-to-output-index mapping for publication. Bake builder-local positions and correct outward normals; retain hard architectural corners, smooth curved patches and calculate tangents from generated UVs using engine conventions. Keep empty groups in the recipe. Treat an empty volume as zero visible geometry and carry this state to step 6. Distinguish CPU preparation from native resource realization at a safe render boundary.

## Acceptance evidence

Verify index ranges, complete triangle coverage with no duplicates, valid normals/tangents and expected slot order for default-only, mixed and empty results. A second material pass must not move vertices or change UVs.

## User validation

If a drawable preview or existing authorized harness is available, inspect a split wall with two ordinary materials and a checkerboard on a curved pillar. Otherwise report CPU/structural evidence and repeat in step 8.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [6.1 Attach and register the C++ component](../step_6/substep_1.md)
