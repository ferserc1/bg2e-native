# 1.4 Isolate toolbar customization by workspace

## Entry condition

[1.3 Dispatch workspace shortcuts exactly once](../step_1/substep_3.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

lib/src/bg2e/ui/{ToolbarPreferences,ToolbarSettings}.cpp and the workspace toolbar setup.

## Implementation contract

Use one stable preferences path per toolbar. Keep the version=1 file and action IDs. Register eligible actions and default layout before loading preferences. Preserve left/right, separators, hidden buttons and existing recovery for bad files. Apply the existing unknown/duplicate ID cleanup against the eligible catalog. A workspace switch must not overwrite a different toolbar layout.

## Acceptance evidence

Round-trip two distinct layouts, including an empty side; reopen with an obsolete ID. Confirm single-toolbar files still load and changing one leaves the other untouched.

## User validation

Launch Composer, customize its existing toolbar, and restart. Confirm that the layout is restored and the usual menu and toolbar actions work.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [2.1 Extract the existing scene workspace](../step_2/substep_1.md)
