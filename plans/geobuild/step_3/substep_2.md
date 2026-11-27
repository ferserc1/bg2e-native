# 3.2 Integrate increments with the scene translation gizmo

## Entry condition

[3.1 Define shared snapping calculations and preferences](../step_3/substep_1.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

lib/src/bg2e/manipulation/GizmoComponent.cpp translation drag path; Composer TransformInteractions.cpp and workspaces/common.

## Implementation contract

Apply the shared calculation to the constrained translation delta before publishing the new transform. Convert distances through the parent transform so scene-unit steps remain meaningful under transformed parents. Preserve orientation, scale and existing gizmo behavior with snapping disabled. Expose toggle, step and alignment choice through Composer. One drag is one history interaction, including any reversals; cancel restores the original matrix. Use the narrowest additive configuration needed for general gizmos.

## Acceptance evidence

Check all supported translation handles, parent transforms, positive/negative travel, cancel, undo/redo, and repeated frames at a stationary cursor. Confirm scale and rotate are unaffected.

## User validation

Place a node off-grid, move it by three increments and back one, undo once, then try absolute alignment. Repeat under a rotated parent and with snapping disabled.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [4.1 Define the editable CPU recipe](../step_4/substep_1.md)
