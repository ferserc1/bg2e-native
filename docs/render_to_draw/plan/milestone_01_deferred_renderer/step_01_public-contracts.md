# M01-01 — Public object families and configuration contracts

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** Plan root decisions.

## Scope and deliverable

- `lib/include/bg2e/draw/all.hpp`
- `lib/include/bg2e/draw/Renderer.hpp`
- `lib/include/bg2e/draw/pipeline/RenderLayer.hpp`
- `lib/include/bg2e/draw/environment/all.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/draw/all.hpp](../../../../lib/include/bg2e/draw/all.hpp).

## Implementation work

1. Define public Renderer, renderer configuration and compatible public pipeline/environment object families. Keep actual data in their objects, expose accessors and specify ownership for injected shared/borrowed components.
2. Specify single-sample output/internal extents, formats, pass inputs and scene lifecycle. No native commands/descriptor sets or MSAA fields in the new high-level contracts.
3. Define simple production/draw initialization-mode rules for shared high-level objects and the later deletion boundary; do not redesign Drawable into an external ResourceManager.
4. List configuration compatibility checks needed for layer/effect/shader replacements. Public advanced namespace is draw::pipeline; specialized environment tools use draw::environment.

## Verification and exit conditions

- Static review can trace a user-provided compatible layer/pass from configuration into the virtual recording call.
- Headers do not pull production render wrappers into new core interfaces; no new private class without a specific necessity.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M01-05, M01-06, M01-10 and all bake/RT steps depend on these exact ownership and input contracts.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
