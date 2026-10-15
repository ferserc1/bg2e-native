# M01-13 — Open upscaler API, resolution reduction and FSR integration

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M01-02–M01-05/M01-10/M01-12.

## Scope and deliverable

- `lib/src/bg2e/render/deferred/FSRPostProcessor.cpp`
- `lib/src/bg2e/render/deferred/MotionVectorGenerator.cpp`
- `lib/include/bg2e/draw/Upscaler.hpp`
- `lib/include/bg2e/draw/pipeline/FinalPostProcessor.hpp`
- `lib/include/bg2e/gpu/vk/FidelityFXBridge.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/src/bg2e/render/deferred/FSRPostProcessor.cpp](../../../../lib/src/bg2e/render/deferred/FSRPostProcessor.cpp), [lib/src/bg2e/render/deferred/MotionVectorGenerator.cpp](../../../../lib/src/bg2e/render/deferred/MotionVectorGenerator.cpp).

## Implementation work

1. Define a public virtual upscaler with stable identifier/label, reflected settings/accessors, extent/input requirements, capability status and recording/reset lifecycle. Allow external application injection/registration without an engine-closed enum.
2. Implement default adjustable internal resolution plus fullscreen resampling. Keep lower resolution even when FSR is unsupported; use single-sample targets and no engine histories.
3. Adapt FSR using minimal camera/depth motion vectors/jitter and its own SDK contexts. Keep native handler casts and SDK Vulkan implementation inside a narrow gpu::vk bridge; draw owns quality/settings/pass policy.
4. Declare SDK/backend availability separately from GPU hardware support. On failed initialization or mode changes preserve a usable default output and report status. Native Metal upscaling and DLSS remain unimplemented extension cases.

## Verification and exit conditions

- Native and reduced-resolution default output works on Metal/Vulkan without FSR; switching to/from supported FSR yields valid output and settings enumeration.
- A small application-defined resampling upscaler can be injected and configured through reflection without modifying engine enumeration.
- FSR state/transition/completion contracts are reconciled; no TemporalAccumulator/DenoiseFilter is implemented in M01.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M03 may enhance motion/history resets but must not close this API. M05/M06–M08 import settings and use component identifiers established here.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-04 shader deployment handoff

Use the paired source/output paths and actual ShaderLib entry points documented in [shader-libraries.md](shader-libraries.md). The registered recursive draw shader tree already handles shared includes, native compilation and app/install/standalone deployment. Add concrete pipeline stages there; do not flatten outputs or treat the `probe` libraries as the final renderer implementation. Keep Vulkan `main`, Metal stage-specific entry points and explicit backend resource indices.

## M01-05 lifecycle handoff

M01-05 resolves zero initial internalExtent to output, while explicit internalExtent stays fixed on resize. Extend that startup rule to consult the renderer's live internal-resolution/upscaler policy when applying new output extents; do not cache a parallel settings value in the delegate. Use sceneFrameNumber for FSR scene submissions and reset SDK history at renderer/scene/extent boundaries. See [runtime-renderer.md](runtime-renderer.md).

## M01-10 actual final output

The ordinary fallback already uses public FinalLayer/FullscreenPass with explicit four-tap bilinear reconstruction from internal HDR to output extent and one final tone/color correction. See [raster-layers.md](raster-layers.md). Introduce the open Upscaler and live scale policy here without discarding this no-SDK/no-RT fallback. Split/reuse final reconstruction and color processing as needed for SDK color-space requirements; never tone-map each transparent item. Final pass currently takes color plus seven G-buffer debug sources and exposes only0..7 debug modes. RendererDeferred slot targets include preserved opaque depth and working last-layer depth; choose/document correct motion/depth inputs, rather than mistaking the final transparent depth for opaque depth. No previous-object matrices/history are supplied yet.

## M01-12 actual catalog extension

Use [reflected-settings.md](reflected-settings.md): RenderSettingsPreferences::bind/connect accept ExtendCatalog for real upscaler/scale objects. Append stable component/type/property IDs and retained owners using SettingsTarget; instance options/ranges/availability belong to copied TypeInfo metadata, not shared TypeRegistry mutations. Queue live edits through the pre-acquire delegate operation; any resource/graph change must detach/rebind and invalidate catalog generations. Do not introduce a copied upscaler preferences model. Typed JSON uses numeric strings and int64 enum identities, never option indices. Preserve FinalLayer's live color settings while changing reconstruction policy.

## M01-13 implementation handoff

Actual public APIs, graph/native ABI, SDK availability/state/lifetime, reset/fallback limits and lead validation are recorded in [upscaling.md](upscaling.md). RendererDeferred now uses finalContract(output, depth, true); standalone final.v1 remains available. The lead validates this delivery before M01-14.
