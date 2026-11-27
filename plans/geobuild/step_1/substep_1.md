# 1.1 Define workspace masks and active workspace

## Entry condition

Initial plan accepted for implementation and the user requests the first substep. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

lib/include/bg2e/ui/{Workspace,ActionDefinition}.hpp and their implementations.

## Implementation contract

Keep Workspace as the layout of borrowed windows. Add a UI-level collection with stable single-bit slots Workspace0 through Workspace31 and WorkspaceAll covering all 32 bits. Provide registration and activation by a single registered bit. Append workspace membership to ActionDefinition with WorkspaceAll as its default. Multiple workspaces per action use bitwise union. Keep the active slot per collection.

## Acceptance evidence

Check bit 31 without signed shifts, union membership, repeated activation as a no-op, and rejection of zero, multiple or unknown slots for activation. Existing aggregate definitions and single-workspace callers retain their behavior.

## User validation

No standalone application interaction is available at this boundary. Give the user the specific contract and verification evidence below for review.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [1.2 Project actions into menus and toolbars](../step_1/substep_2.md)
