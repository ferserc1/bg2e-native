# M04-01 — Bake namespace and public configuration/pixel contracts

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** Accepted M03 review.

## Scope and deliverable

- `lib/include/bg2e/render/LightmapSettings.hpp`
- `lib/include/bg2e/render/LightmapBaker.hpp`
- `lib/include/bg2e/bake/LightmapSettings.hpp`
- `lib/include/bg2e/bake/LightmapPixels.hpp`
- `lib/include/bg2e/bake/BakerContext.hpp`
- `lib/include/bg2e/bake/all.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/LightmapSettings.hpp](../../../../lib/include/bg2e/render/LightmapSettings.hpp), [lib/include/bg2e/render/LightmapBaker.hpp](../../../../lib/include/bg2e/render/LightmapBaker.hpp).

## Implementation work

1. Define all new map-baking configuration/resources/results under bake, including target/base contexts, UV preview and eventual standalone assembly/batch declarations as needed.
2. Specify LightmapPixels byte/float RGB formats, size/orientation/color semantics and LightmapSettings source-of-truth accessors/configuration. UI/persistence enumerate actual bake object settings, not duplicate values.
3. Expose compatible public draw::pipeline effect/surface/filter object injection and explicit ownership in bake configuration structs.
4. Record the complete legacy-to-bake mapping, including private executor/UV/composition/accumulation responsibilities, without moving UV2 geometry preprocessing.

## Verification and exit conditions

- Public declarations can express AO/GI and future map generation inputs without native Vulkan descriptors or a mandatory RendererDeferred.
- No implementation depends on future standalone APIs merely to run an integrated bake.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M04-03/M04-04 implement these values/lifetimes. M08 completes StandaloneBakerContext/SceneAssembler/Batch in bake, not DB or draw.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
