# M03-06 — Public traced reflections and resolution semantics

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M03-01–M03-03/M03-05 shared hit data.

## Scope and deliverable

- `lib/src/bg2e/render/deferred/RTReflections.cpp`
- `lib/include/bg2e/render/deferred/RTReflections.hpp`
- `lib/include/bg2e/draw/pipeline/RTReflections.hpp`
- `shaders/src/draw/rt_reflections/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/src/bg2e/render/deferred/RTReflections.cpp](../../../../lib/src/bg2e/render/deferred/RTReflections.cpp), [lib/include/bg2e/render/deferred/RTReflections.hpp](../../../../lib/include/bg2e/render/deferred/RTReflections.hpp).

## Implementation work

1. Implement explicit backend reflection shaders producing HDR radiance and mask/confidence, with cutout-aware geometry/material hit lookup.
2. Preserve roughness threshold/spread, sample count, bias/distance, quality, shadow-sample/noise and reflection-light participation settings.
3. Keep reflection-quality extent relative to display output, independently of internal G-buffer/FSR scale. Define IBL fallback when traced data is unavailable.
4. Expose compatible shader/resource/subclass configuration and normal accessors; no native Vulkan descriptors in public reflection objects.

## Verification and exit conditions

- Reflective/rough surfaces, misses and alpha-cutout hits compose correctly; output-size-only and internal-scale changes resize the appropriate resources.
- Fallback retains environment specular behavior, and user component injection follows the public pass contract.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M03-07 owns reflection histories separately from GI/AO. M05 freezes scene while collecting requested rendered-frame samples.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
