# M08-03 — Bake batch/output writer and protected complete exports

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M08-01/M08-02 and M04 pixel contracts.

## Scope and deliverable

- `lib/include/bg2e/render/StandaloneBakeBatch.hpp`
- `lib/src/bg2e/render/StandaloneBakeBatch.cpp`
- `lib/include/bg2e/db/LightmapOutputWriter.hpp`
- `lib/src/bg2e/db/LightmapOutputWriter.cpp`
- `lib/include/bg2e/bake/StandaloneBakeBatch.hpp`
- `lib/include/bg2e/bake/LightmapOutputWriter.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/StandaloneBakeBatch.hpp](../../../../lib/include/bg2e/render/StandaloneBakeBatch.hpp), [lib/src/bg2e/render/StandaloneBakeBatch.cpp](../../../../lib/src/bg2e/render/StandaloneBakeBatch.cpp), [lib/include/bg2e/db/LightmapOutputWriter.hpp](../../../../lib/include/bg2e/db/LightmapOutputWriter.hpp), [lib/src/bg2e/db/LightmapOutputWriter.cpp](../../../../lib/src/bg2e/db/LightmapOutputWriter.cpp).

## Implementation work

1. Place specialized bake batch execution and output/preflight/model-copy orchestration in bake, including LightmapOutputWriter; keep generic image/scene file functions in DB.
2. Preserve complete-batch output planning, duplicate identities/path collision checks, protected input assets/sidecars, explicit overwrite policy and atomic replacement semantics.
3. Write LightmapPixels RGB8/RGB32F maps with format/conversion/dilation metadata and optional model copies that reference the generated resource correctly.
4. Reuse complete high-level save/readback paths; preserve production DB writer entry points as thin legacy-compatible overloads during coexistence rather than break the old CLI early.

## Verification and exit conditions

- Preflight catches target/output/input collisions before baking/export, and overwrite permission is honored precisely.
- Byte/float outputs and model copies reload with correct resource/UV/multiplier semantics; no original source asset is overwritten accidentally.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M08-04 consumes this orchestration. M09 removes production writer wrappers and all render::LightmapPixels dependencies; bake remains the owner.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
