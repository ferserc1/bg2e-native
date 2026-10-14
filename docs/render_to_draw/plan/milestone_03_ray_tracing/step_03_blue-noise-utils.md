# M03-03 — Generic BlueNoise data and GPU resource in utils

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M01 image/subresource primitives and M03-02.

## Scope and deliverable

- `lib/src/bg2e/render/BlueNoise.cpp`
- `lib/src/bg2e/render/blue_noise_data.h`
- `lib/include/bg2e/utils/BlueNoise.hpp`
- `lib/src/bg2e/utils/BlueNoise.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/src/bg2e/render/BlueNoise.cpp](../../../../lib/src/bg2e/render/BlueNoise.cpp), [lib/src/bg2e/render/blue_noise_data.h](../../../../lib/src/bg2e/render/blue_noise_data.h).

## Implementation work

1. Implement generic precomputed sample/noise resource use in utils, exposing tile/channel/layer metadata and GPU image/sampler access without production Engine/native Vulkan objects.
2. Choose a GPU Device or draw Engine overload as the simplest residency interface; keep CPU sample data usable where meaningful without initialization.
3. Complete primitive 2D-array image/layer upload/view support if still missing; native array operations belong in GPU. Preserve explicit shader-side sampling conventions.
4. Allow effect/bake callers to share explicitly owned noise resources; rare replacement uses safe completion rather than per-frame texture rings. Keep production BlueNoise untouched until cleanup.

## Verification and exit conditions

- Noise tiles/layers are sampled correctly on Vulkan/Metal and are reusable by a utility caller outside RendererDeferred.
- A missing/disabled noise resource yields the declared alternative sampling mode; ownership survives renderer/effect replacement.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M03 effects and M04 baking reuse utils::BlueNoise rather than add draw/bake-specific duplicate data families.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
