# M08-02 — Bake model/prefab/context assembly and UV2

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M08-01 and accepted M06/M07 DB contracts.

## Scope and deliverable

- `lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp`
- `lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp`
- `lib/include/bg2e/bake/StandaloneBakeSceneAssembler.hpp`
- `lib/include/bg2e/geo/GenerateUv2AtlasModifier.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp](../../../../lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp), [lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp](../../../../lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp), [lib/include/bg2e/geo/GenerateUv2AtlasModifier.hpp](../../../../lib/include/bg2e/geo/GenerateUv2AtlasModifier.hpp).

## Implementation work

1. Reimplement assembly in bake using pure schema/file IO plus high-level draw resource load where needed. Preserve context/model/prefab root rules, target/source identities and optional load-target-resource behavior.
2. Keep UV2 atlas generation/validation in geo, at the same coordinated waitIdle/rebuild boundary as editor/bake examples.
3. Capture complete geometry/material/environment/light inputs without viewport tool/gizmo contamination; preserve joints/metadata and required target eligibility rules.
4. Do not create a parallel parser or ResourceManager; reuse actual component factory/load/save contracts from application milestones.

## Verification and exit conditions

- Model and prefab batches select the intended targets and stable output identities, with correct source paths and UV2 metadata.
- Missing resources/invalid context/unbakeable targets fail with meaningful diagnostics before partial output publication.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M08-03 preflight protects these source identities and writes model copies. M09 neutral DB/factory contracts must retain their final resource readiness behavior.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
