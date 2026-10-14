# M04-03 — Integrated context and shared AO/GI bake execution

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M04-01/M04-02 and M03 effects.

## Scope and deliverable

- `lib/include/bg2e/render/IntegratedBakerContext.hpp`
- `lib/src/bg2e/render/IntegratedBakerContext.cpp`
- `lib/include/bg2e/bake/IntegratedBakerContext.hpp`
- `lib/include/bg2e/bake/LightmapBakeExecutor.hpp`
- `shaders/src/bake/rtao_uv/`
- `shaders/src/bake/rtgi_uv/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/IntegratedBakerContext.hpp](../../../../lib/include/bg2e/render/IntegratedBakerContext.hpp), [lib/src/bg2e/render/IntegratedBakerContext.cpp](../../../../lib/src/bg2e/render/IntegratedBakerContext.cpp).

## Implementation work

1. Implement public IntegratedBakerContext/IntegratedLightmapBaker using experimental command/frame contexts and bake-owned snapshots/TLAS/bindings where isolation is required.
2. Validate prepared command, slot, target scene generation and one-sample-per-frame rules. Borrow/share explicit environment resources with lifetime checks, rather than initialize a second renderer.
3. Reuse public draw AO/GI implementations/configurations with the UV profile and explicit backend shader variants. Allow compatible application replacements.
4. Define edit/cancel/reset behavior and retain sources until submitted work completes. UI/editor gizmos never contaminate the traceable bake scene.

## Verification and exit conditions

- Prepared-frame mismatches, duplicate updates, removed targets and scene edits are rejected or reset according to the declared contract.
- An integrated bake runs alongside scene presentation without overwriting raster/selection descriptor or tracing resources.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M04-06 and M06/M07 use these lifecycle hooks. M08 implements a different standalone scheduler over the same public bake execution pieces.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
