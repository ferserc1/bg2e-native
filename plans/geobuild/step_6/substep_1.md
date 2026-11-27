# 6.1 Attach and register the C++ component

## Entry condition

[5.4 Assemble the ordinary Drawable](../step_5/substep_4.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

lib/include/bg2e/scene/{Component,ComponentFactoryRegistry,DrawableComponent}.hpp, built-in component registration and scene umbrella headers.

## Implementation contract

Expose GeometryBuilderComponent as a normal scene component referring to the geobuild recipe. Serialize under one stable type name GeometryBuilderComponent, following the built-in type-name convention for registration and reading. Keep geometry ownership in a separate ordinary DrawableComponent on the same node. Enforce one recipe and at most one owner for its output. Creating a builder on an unrelated imported owner must never adopt/overwrite that geometry implicitly. Clone the recipe and group data by value; editor sessions are transient. Loading an already baked scene should not trigger evaluation.

## Acceptance evidence

Trace API creation, built-in registration, lookup, cloning and node compatibility. A normal scene without a builder follows the previous code paths. Confirm the generic scene transform remains editable on a builder owner; account for the Composer transform allowlist when wiring the UI.

## User validation

No standalone application interaction is available at this boundary. Give the user the specific contract and verification evidence below for review.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [6.2 Serialize the versioned recipe and surface assignments](../step_6/substep_2.md)
