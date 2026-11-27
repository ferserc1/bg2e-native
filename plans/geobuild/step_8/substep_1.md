# 8.1 Enter and leave a volume editing session

## Entry condition

[7.4 Resize primitives using opposite-face anchoring](../step_7/substep_4.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

Composer workspaces/volume, scene inspector, AppDelegate, Document and the transition hook from step 2.

## Implementation contract

Add create-builder on a new node and Edit Volume for a builder owner. Start a private draft from the confirmed recipe; retain document identity, owning node, generation, last confirmed state and camera. Keep Geometric Editing and Material Groups as tools of the same workspace. Define session-local undo/redo for draft operations, routed by the common actions. Expose Cancel and restore the scene selection to the owner. Wire destructive exit/close through normal document handling. Until publication lands, enable only actions that can complete correctly. Keep private helpers outside authored traversal, export and resource browsers.

## Entry and ownership

For a new builder start with one centered unit cube, seed operation and no explicit groups. Keep new-node creation consistent with normal scene commands. For an existing builder make a private draft copy. Track which output corresponds to the last committed recipe. The active operand is an ID in the draft, not a node in the authored scene. A mode switch must release stale keyboard focus, mouse capture, overlays and selection callbacks.

## Acceptance evidence

Enter/exit/reenter with clean and dirty documents, move camera, cancel, verify history boundaries and no leaked helpers. Guard stale session callbacks when a document is closed or replaced.

## User validation

Open an existing scene with an imported mesh, start a volume and orbit around it. Cancel and verify scene, camera position, prior unsaved status and owner selection.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [8.2 Edit and preview an ordered list of operands](../step_8/substep_2.md)
