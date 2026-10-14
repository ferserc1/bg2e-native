# M01-13 — Open upscaler API, resolution reduction and FSR integration

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M01-02–M01-05/M01-10/M01-12.

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

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M03 may enhance motion/history resets but must not close this API. M05/M06–M08 import settings and use component identifiers established here.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
