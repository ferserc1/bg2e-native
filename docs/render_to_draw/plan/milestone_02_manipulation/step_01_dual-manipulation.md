# M02-01 — Dual-mode manipulation objects and scene references

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** Accepted M01 review.

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
3. Keep engine-independent SelectableComponent and transform intent unchanged. GizmoComponent creates draw Drawable geometry in the new path and clones runtime handles according to existing semantics.
4. Define renderer-independent attachment of optional editor tooling; headless rendering later requires no selection manager.

## Verification and exit conditions

- Selection references expire safely after node/component deletion and never reinterpret a legacy mesh as draw.
- Current editor/legacy consumers still compile under authorized verification; draw mode rejects unsupported cross-mode operations clearly.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M02-02/M02-03/M02-07 consume this initialized-mode model. M06/M07 migrate app callbacks; M09 removes its production branches.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
