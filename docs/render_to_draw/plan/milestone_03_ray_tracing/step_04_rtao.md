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

## M03-01 checkpoint handoff — accepted by lead continuation

After acceptance of [M03-01](rt-design.md), introduce public draw::pipeline tracing snapshot and surface-input contracts here, including the CPU packed-geometry/material builder and retained owners. GPU primitives come from M03-02; no draw policy belongs in GPU. Use dense TLAS order, explicit submesh index/vertex bases, 192-byte MaterialData semantics and bounded independent image/sampler tables. AO uses compute queries independently of native Vulkan pipeline support, evaluates cutouts with the same predicate used by later effects, returns neutral 1 and validates scene capacity before publishing. Expose camera-depth and explicit world-position surface profiles for bake reuse. This step precedes GI/reflections; do not depend on a table builder scheduled after AO.

## M03-02 actual primitive contract

Read [rt-gpu-data.md](rt-gpu-data.md) with the later [M07 capacity amendment](../milestone_07_bg2e_composer/tracing-capacity-16384.md). Choose the exact device-clamped capacity up to 16384 after reserves, not small compiled effect profiles. Vulkan specializes ID0; Metal uses separate argument record buffers8/9. Keep complete arrays, retained owners, retired/drained updates and warning/neutral-texture overflow behavior. Primitive probe profiles are not the engine ABI. No global material ownership, buffer-address or array graphics-stage infrastructure is implied.

## M03-03 actual noise input

Reuse [utils::BlueNoise](blue-noise.md): CPU layer-major rank data and an explicitly shared RGBA8 color 2D atlas, with uint4 metadata and paired bg2eBlueNoise2/bg2eHashNoise2 helpers under draw/lib. Logical layers are stacked in Y; physical arrayLayers is one. Do not copy production sampler2DArray bindings or assume general image-array support. Effects own rendered-view/sample keys and disabled/missing-noise variants without texture access; the utility does not advance presentation/slot state or reserve tracing-table capacity implicitly. Retain image/sampler owners and metadata snapshots through completion.

GLSL noise calls must pass texture2D and sampler separately to bg2eBlueNoise2; its internal texelFetch constructs sampler2D at point of use. Do not pass a combined sampler constructor through a user-function argument.

## M03-04 delivered contract

See [rtao.md](rtao.md) for actual public APIs, native bindings, ownership, composition, fallback and limitations, and the [manual fixture](../../../../tests/draw/rtao/README.md). Delivery awaits project lead compilation/runtime validation.
