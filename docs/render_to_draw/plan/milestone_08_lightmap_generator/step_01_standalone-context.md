# M08-01 — Public standalone bake context and scene generation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** Accepted M04/M05/M07 reviews.

## Scope and deliverable

- `lib/include/bg2e/render/StandaloneBakerContext.hpp`
- `lib/src/bg2e/render/StandaloneBakerContext.cpp`
- `lib/include/bg2e/bake/StandaloneBakerContext.hpp`
- `lib/include/bg2e/bake/StandaloneLightmapBaker.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/StandaloneBakerContext.hpp](../../../../lib/include/bg2e/render/StandaloneBakerContext.hpp), [lib/src/bg2e/render/StandaloneBakerContext.cpp](../../../../lib/src/bg2e/render/StandaloneBakerContext.cpp).

## Implementation work

1. Implement standalone Created/Initialized/SceneReady/Cleaned lifecycle and explicit scene preparation using a windowless draw engine/device, independent of a camera RendererDeferred.
2. Capture environment/light/material/geometry data and prepare isolated TLAS/resources using public draw/bake building blocks.
3. Bind baker instances to scene generation, reject stale/duplicate/invalid updates and define synchronous submission/completion for one accumulation sample.
4. Preserve target validation, root ownership, cleanup and producer coordination. No-RT hardware reports unsupported required baking instead of silently outputting unbaked maps.

## Verification and exit conditions

- A prepared context can bake multiple targets and invalidates prior bakers after a new scene generation.
- No main camera/window/UI or hidden production renderer is required to create resources; exceptions/cleanup do not leave active work.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M08-02/M08-04 use these types. M09 deletes the legacy standalone implementations only after equivalence is evidenced.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
