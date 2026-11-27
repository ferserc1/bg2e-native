# 8.6 Apply, cancel and save the active draft

## Entry condition

[8.5 Edit surface groups with pastel previews](../step_8/substep_5.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

Composer DocumentIo.cpp, workspaces/volume, command dispatcher, replacement and save code from step 6.

## Implementation contract

Apply settles gestures, evaluates the current draft, prepares recipe and owner publication, then commits them together and returns to Scene. Clear history only on successful workspace change. Cancel drops edits since entry or last successful save. Common Save/Save As in Volume persists the validated draft and scene and stays in Volume; establish a new cancel baseline after successful I/O. Resolve destination before mutating a Save As draft. Coordinate existing staged save and resource changes with snapshots/rollback at a safe boundary. On cancellation or failure retain recoverable draft, prior confirmed state, source path and correct dirty status; surface existing incomplete-save diagnostics if disk publication partially failed. When no draft is pending preserve normal Save behavior. Ensure undo history in Volume cannot restore stale references across a save baseline.

## Acceptance evidence

Verify failed boolean, failed save, cancelled Save As, instance counts, keyboard invocation once, no history loss on failure and correct dirty status. Test both empty and nonempty legal publications.

## User validation

Save at 900 mm, change to 950 mm, Cancel and reopen. Expect 900 mm. Apply a second edit and inspect normal materials, including an instance; check file paths after Save As.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [8.7 Validate the complete industrial asset workflow](../step_8/substep_7.md)
