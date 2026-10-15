# M02-01 — Dual-mode manipulation objects and scene references

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** Accepted M01 review.

## Scope and deliverable

- `lib/include/bg2e/manipulation/SelectionManager.hpp`
- `lib/include/bg2e/manipulation/GizmoComponent.hpp`
- `lib/include/bg2e/manipulation/SelectableComponent.hpp`
- `lib/include/bg2e/scene/DrawableComponent.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/manipulation/SelectionManager.hpp](../../../../lib/include/bg2e/manipulation/SelectionManager.hpp), [lib/include/bg2e/manipulation/GizmoComponent.hpp](../../../../lib/include/bg2e/manipulation/GizmoComponent.hpp), [lib/include/bg2e/manipulation/SelectableComponent.hpp](../../../../lib/include/bg2e/manipulation/SelectableComponent.hpp), [lib/include/bg2e/scene/DrawableComponent.hpp](../../../../lib/include/bg2e/scene/DrawableComponent.hpp).

## Implementation work

1. Add draw Engine initialization/constructor overloads and explicit mode state to SelectionManager/GizmoComponent/overlay integration. Preserve production overloads rather than redefine the entire interaction model.
2. Extend SelectionItem and accessors to represent either legacy or draw geometry with weak lifetime and typed access; preserve submesh IDs, selected-node deduplication and callback semantics.
3. Preserve SelectableComponent selection intent but implement mode-aware update: the current production drawableBase() call throws for draw. Use executionMode()/drawDrawable(), implement graphics-free deserialize/serializeReferences hooks, preserve fresh per-clone IDs and define submesh-count/resource replacement behavior. GizmoComponent creates draw Drawable geometry in the new path; retain production behavior.
4. Define renderer-independent attachment of optional editor tooling; headless rendering later requires no selection manager.

## Verification and exit conditions

- Selection references expire safely after node/component deletion and never reinterpret a legacy mesh as draw.
- Current editor/legacy consumers still compile under authorized verification; draw mode rejects unsupported cross-mode operations clearly.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M02-02/M02-03/M02-07 consume this initialized-mode model. M06/M07 migrate app callbacks; M09 removes its production branches.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-07 schema handoff

SelectableComponent currently inherits only the production Component parser; the new typed schema/draw factory rejects it until its graphics-free parser and serializeReferences contract are implemented. Add those trivial attribute hooks alongside the actual draw-aware selection/update work here, retaining per-instance runtime identifiers. See [db-levels.md](../milestone_01_deferred_renderer/db-levels.md). Raw scene document IO already retains its JSON without constructing it.

## M02-01 actual handoff

Delivered contracts and validation are recorded in [dual-manipulation.md](dual-manipulation.md). Execution status remains in progress.md.
