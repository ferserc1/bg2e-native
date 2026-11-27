# 8.7 Validate the complete industrial asset workflow

## Entry condition

[8.6 Apply, cancel and save the active draft](../step_8/substep_6.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

Composer Scene/Volume workspaces; existing scene I/O and standalone model export; existing C++ and TypeScript consumers.

## Implementation contract

Use an imported reference and two independent coplanar builder walls. Include a full-height opening, ordinary window, jambs, cylinder and sphere, plus an instance with independent materials. Exercise grid, cycling selection, grouping, common settings, per-workspace toolbar customization, save/reopen, standalone export and C++ re-edit. Address only integration regressions attributable to the plan. Record platform, assets, observed outcomes and unavailable consumers.

## Evidence to retain in progress

Record the two consumers and their versions when available, the exported model path and resources used, and which checks the user performed. Include at least one owner with more material slots after regeneration and one with fewer. Compare the standalone model against the confirmed scene appearance using the same ordinary materials. External SDK or platform validation explicitly deferred by the user remains visible in the final log.

## Acceptance evidence

Check full-height split vs connected window front, one material on disconnected regions, fixed UVs after moving the drawable, scene realignment after geometric regeneration, independent asset pivot and texture references. Test index-based truncation/extension and equal-count instance semantics. Verify both toolbars after restart and only the active Delete shortcut.

## User validation

Run the fixture in Composer, reopen in C++, load the same scene in TypeScript and import the isolated wall into an empty scene. Judge dimensional control, curved UVs, pastel readability and agreement between preview and baked mesh. Report actual passes and any deferred external verification.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — End of plan. Mark COMPLETE only after explicit user acceptance.
