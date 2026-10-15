# M03-05 — Public RTGI and equivalent backend kernels

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M03-01–M03-03; shared effect surface/resource contracts.

## Scope and deliverable

- `lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp`
- `shaders/src/glsl/rt_gi.rgen.glsl`
- `lib/include/bg2e/draw/pipeline/RTGlobalIllumination.hpp`
- `shaders/src/draw/rtgi/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp](../../../../lib/src/bg2e/render/deferred/RTGlobalIllumination.cpp), [shaders/src/glsl/rt_gi.rgen.glsl](../../../../shaders/src/glsl/rt_gi.rgen.glsl).

## Implementation work

1. Implement GI using the accepted Vulkan RT stage arrangement and equivalent Metal compute/intersection shaders; share logical material/light/environment inputs, not shader source.
2. Expose sample/bounce/quality/bias/distance/shadow-sample/noise accessors and replaceable compatible components/shaders.
3. Preserve RTAO versus RTGI mode selection and production hit-material baseline, including emission, albedo UV/cutouts and environment/light participation.
4. Produce a typed HDR indirect signal with defined neutral/unsupported output. Keep scene traversal and tables coherent with raster edits and reserve surface-profile reuse for bake.

## Verification and exit conditions

- GI and emission/bounce effects are visible on supported backends; cutouts and nested transforms agree with query/raster geometry.
- Disabled/unsupported GI restores ordinary IBL/baked-map composition and settings availability without corrupting requested configuration.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M03-07/M03-08 supply HDR history/denoise; M04/M08 consume the effect independently of a camera renderer.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
