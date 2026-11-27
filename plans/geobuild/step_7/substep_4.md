# 7.4 Resize primitives using opposite-face anchoring

## Entry condition

[7.3 Pick logical regions on the evaluated mesh](../step_7/substep_3.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

geobuild dimension utilities and the shared grid calculation from step 3.

## Implementation contract

Use cube faces, cylinder caps plus four lateral box handles, and six sphere box handles. All operate on canonical primitive limits under the original affine matrix. Move the chosen limit along its transformed local axis and fix the opposite face in world space; preserve other transformed axes and affine shear. Convert requested world displacement to an axis parameter via its world length. Update center and extent from the gesture baseline. Reflections retain their handedness; stop before crossing zero and use a positive finite minimum extent in scene units. A numeric final dimension uses the same anchor and constraint. Apply the relative snap rule or eligible absolute alignment from step 3.

## Anchor calculation

For a positive-side handle, increasing the dimension by d moves the center by +d/2 along the positive transformed axis; for a negative-side handle it moves by -d/2. Measure distances between corresponding handle centers under a sheared transform. Change the selected basis column by newLength / oldLength and update translation by half the signed displacement. Invert the parent/builder transform once to write back to recipe-local coordinates. Direct numeric input specifies an exact positive length even with grid snapping enabled. Reject nonpositive numeric input; clamp drag extents to a minimum of 0.000001 scene units.

## Acceptance evidence

Check a rotated 900 mm doorway, one anchored jamb, an elliptic cylinder and ellipsoid, a scaled parent, repeated drags and reversal/cancel. Verify the opposite plane position and grid-step displacement numerically.

## User validation

No standalone application interaction is available at this boundary. Give the user the specific contract and verification evidence below for review.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [8.1 Enter and leave a volume editing session](../step_8/substep_1.md)
