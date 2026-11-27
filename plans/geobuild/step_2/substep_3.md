# 2.3 Coordinate transitions and clear history safely

## Entry condition

[2.2 Classify common and scene-only actions](../step_2/substep_2.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

Composer AppDelegate, EditorUi, Document, TransformInteractions, command dispatcher and lib/include/bg2e/app/CommandHistory.hpp.

## Implementation contract

Implement a Composer-owned transition sequence. Check destination readiness, pending I/O/import/commands and ability to settle an active gesture before committing the switch at a stable boundary. Discard undo and redo on each effective switch using existing history eviction; preserve document generation, saved state and dirty flag. Same workspace requests are no-ops. Failed transitions preserve the workspace and history. Stage/camera, document and renderer remain alive. Do not expose a dummy volume workspace in the released menu.

## Transition boundary

Prepare the destination and verify all preconditions before discarding entries. Use discardOldest(undoCount()) and discardRedo() at an idle dispatcher boundary, with no callbacks or queued edits interleaved. Check the results. resetBaseline changes document generation and is reserved for its existing document-replacement or recovery cases. Use Document::synchronizeDirty after the transition. Opening settings or switching between Geometry and Materials leaves both the workspace and its history intact.

## Acceptance evidence

Check clean and dirty documents with both undo and redo entries, refusal during pending work, same-workspace activation and fresh history on successful entry/exit. Exercise internally until the second workspace is usable.

## User validation

Launch Composer and verify that opening and closing settings preserves the current scene and its undo and redo history. Effective workspace switching becomes available later.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [3.1 Define shared snapping calculations and preferences](../step_3/substep_1.md)
