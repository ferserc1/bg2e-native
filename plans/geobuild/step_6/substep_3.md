# 6.3 Publish the recipe and owner as one change

## Entry condition

[6.2 Serialize the versioned recipe and surface assignments](../step_6/substep_2.md) accepted. Verify that [progress.md](../progress.md) points here. Follow the [execution protocol](../summary.md#execution-protocol). Implement this substep alone.

## Starting points

lib/src/bg2e/scene/{DrawableComponent,drawable_resources}.cpp, draw/DrawableCache and Composer command/scene/ReplaceDrawable.* as replacement precedent.

## Implementation contract

Prepare the result and recipe together before installing either. Retain the existing owner resource identity on nonempty replacement. Preserve owner material slots by output index where present and initialize new slots normally. Follow the existing DrawableInstance replacement path: truncate surplus overrides, extend missing slots with initialized materials, preserve equal-count slots by index regardless of face changes. Check that live and stored definitions agree. If preparation/assignment fails, retain the previous recipe, resource and instance appearances. An empty boolean result is valid. Represent a new committed empty builder by its recipe alone, with no drawable file or visible geometry; for nonempty-to-empty use existing legal owner-removal semantics. If instances prevent removal, report this and keep the edit pending; never weaken owner checks or silently delete instances.

## Acceptance evidence

Verify first publication, same/larger/smaller slot counts with an instance, failure rollback, empty-to-nonempty and permitted/nonpermitted owner removal. Keep debug resource naming and safe GPU publication discipline. Confirm unrelated nodes remain untouched.

## User validation

No standalone application interaction is available at this boundary. Give the user the specific contract and verification evidence below for review.

## Handoff

In [progress.md](../progress.md), keep this file as Current, set AWAITING_USER_VALIDATION, write a short summary of the actual change, record checks and outcomes, and give the user the validation above with expected results. Record the following candidate as Next after validation and stop. On explicit acceptance log this substep, advance the cursor, and give the user a short preview of that successor.

**Next after validation** — [6.4 Verify scene save, standalone export and SDK interoperability](../step_6/substep_4.md)
