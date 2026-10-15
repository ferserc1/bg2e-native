# M03-04 — Public RTAO implementation and fallback

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M03-01–M03-03 and M01 G-buffer/layer contracts.

## Scope and deliverable

- `lib/src/bg2e/render/deferred/RTAmbientOcclusion.cpp`
- `shaders/src/glsl/rt_ao.comp.glsl`
- `lib/include/bg2e/draw/pipeline/RTAmbientOcclusion.hpp`
- `shaders/src/draw/rtao/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/src/bg2e/render/deferred/RTAmbientOcclusion.cpp](../../../../lib/src/bg2e/render/deferred/RTAmbientOcclusion.cpp), [shaders/src/glsl/rt_ao.comp.glsl](../../../../shaders/src/glsl/rt_ao.comp.glsl).

## Implementation work

1. Implement camera G-buffer ray-query AO with explicit Vulkan/Metal shaders using chosen material/cutout data and optional shared BlueNoise.
2. Expose quality, sample/bounce counts, radius/bias/falloff/attenuation and accessors. Public configuration accepts compatible shaders/resources, with inputs decoupled from a concrete scene renderer.
3. Preserve neutral AO=1 and material/baked/IBL semantics; do not allocate unsupported resources on no-RT devices.
4. Define reusable surface input records sufficient for later UV-space use, without adding bake objects or using fake camera projections here.

## Verification and exit conditions

- AO samples respond to cutouts/occluders/radius and quality settings, and fallback composition retains material AO/IBL.
- A user-provided compatible subclass/shader can replace the default effect; invalid signal format/input profiles are diagnosed.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M03-07/M03-08 add scalar history/filtering. M04 maps its UV surface profile onto this public surface-input contract.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
