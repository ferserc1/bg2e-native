# 7.1 Build viewport rays in geobuild

## Entry condition

[6.4 Verify scene save, standalone export and SDK interoperability](../step_6/substep_4.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

geobuild CPU utilities, lib/include/bg2e/math/projections.hpp and the existing ray convention in manipulation/GizmoComponent.cpp as reference.

## Implementation contract

Expose high-level query inputs for cursor, viewport rectangle, view/projection matrices and builder-to-world matrix. Use logical coordinates with a top-left origin including viewport offset; use a consistent pixel-center convention and the projection Y orientation used by the actual viewport. Convert to the engine zero-to-one depth convention. Unproject near/far using the actual projection, including reversed depth if present; handle infinite far by using a finite interior sample for direction. Support perspective and orthographic views. Transform a normalized world ray to local space while retaining world-distance comparison. Reject singular matrices, invalid viewport and outside pixels without an invalid hit. Keep camera, renderer, picking buffer and mouse event types out of the utility.

## Acceptance evidence

Project known world points then pick them at screen center/corners, with a subviewport, HiDPI logical units, nonidentity parents and nonuniform scale. Distances are from the world near-plane ray origin; any optional occlusion limit must use the same units and origin.

## User validation

No standalone application interaction is available at this boundary. Give the user the specific contract and verification evidence below for review.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [7.2 Pick all intersected original operands](../step_7/substep_2.md)
