# M03-03 — Generic BlueNoise data and GPU resource in utils

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M01 image/subresource primitives and M03-02.

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
3. Implement genuine 2D-array description/layer/upload/view support if the chosen noise layout requires it: reviewed M01 ImageType only has Image2D/Cubemap and arrayLayers is 1/6. Cubemap/mip APIs are reusable primitives, not existing general array support. Keep native operations in GPU and explicit shader sampling conventions.
4. Allow effect/bake callers to share explicitly owned noise resources; rare replacement uses safe completion rather than per-frame texture rings. Keep production BlueNoise untouched until cleanup.

## Verification and exit conditions

- Noise tiles/layers are sampled correctly on Vulkan/Metal and are reusable by a utility caller outside RendererDeferred.
- A missing/disabled noise resource yields the declared alternative sampling mode; ownership survives renderer/effect replacement.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M03 effects and M04 baking reuse utils::BlueNoise rather than add draw/bake-specific duplicate data families.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M03-01 checkpoint handoff — accepted by lead continuation

Keep BlueNoise independent of the proposed tracing table service in [M03-01](rt-design.md). Texture data and GPU ownership live in utils; effects choose sampling and rendered-view sequencing. Do not consume table-array capacity silently or assume a frame-in-flight slot is a new noise/history sample.

## Actual delivery

[blue-noise.md](blue-noise.md) records utils::BlueNoiseData/sampleNoise2D and Device-backed utils::BlueNoise, plus explicit native sampling helpers and a manual CPU/GPU fixture. The selected layout is a single RGBA8 2D atlas with logical layers stacked in Y, so physical image-array primitives are not needed or implemented. Keep the original ranked bytes and production API untouched during coexistence. Next is M03-04 after lead acceptance.
