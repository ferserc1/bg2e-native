# 1.3 Dispatch workspace shortcuts exactly once

## Entry condition

[1.2 Project actions into menus and toolbars](../step_1/substep_2.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

lib/src/bg2e/{ui/ActionDefinition,ui/Toolbar,app/Shortcuts,app/MainLoop}.cpp.

## Implementation contract

Use one managed binding per effective platform chord, including modifier normalization for CmdOrCtrl. Resolve the action against one snapshot of the active workspace per key event. Keep MainLoop forwarding events normally. Prevent managed toolbars from also auto-registering the same action. Preserve enabled state, text-input policy, key-up semantics and standalone registrations. Use lifetime-safe bindings. Reject overlapping masks for different actions on the same chord before installing the batch; allow disjoint masks.

## Acceptance evidence

Exercise exclusive Delete handlers, common Save, a handler that changes workspace mid-dispatch, focused text input, disabled actions and UI teardown/recreation. Each permitted press invokes exactly once.

## User validation

Launch Composer and confirm that existing shortcuts and text input continue to work normally.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [1.4 Isolate toolbar customization by workspace](../step_1/substep_4.md)
