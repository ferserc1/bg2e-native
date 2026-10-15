# M01-02 — GPU MRT, pipeline state, formats and capabilities

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M01-01.

## Scope and deliverable

- `lib/include/bg2e/gpu/Common.hpp`
- `lib/include/bg2e/gpu/GraphicsPipeline.hpp`
- `lib/include/bg2e/gpu/CommandBuffer.hpp`
- `lib/include/bg2e/gpu/PhysicalDevice.hpp`
- `lib/src/bg2e/gpu/vk/GraphicsPipeline.cpp`
- `lib/src/bg2e/gpu/metal/CommandBuffer.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/gpu/Common.hpp](../../../../lib/include/bg2e/gpu/Common.hpp), [lib/include/bg2e/gpu/GraphicsPipeline.hpp](../../../../lib/include/bg2e/gpu/GraphicsPipeline.hpp), [lib/include/bg2e/gpu/CommandBuffer.hpp](../../../../lib/include/bg2e/gpu/CommandBuffer.hpp), [lib/include/bg2e/gpu/PhysicalDevice.hpp](../../../../lib/include/bg2e/gpu/PhysicalDevice.hpp), [lib/src/bg2e/gpu/vk/GraphicsPipeline.cpp](../../../../lib/src/bg2e/gpu/vk/GraphicsPipeline.cpp), [lib/src/bg2e/gpu/metal/CommandBuffer.cpp](../../../../lib/src/bg2e/gpu/metal/CommandBuffer.cpp).

## Implementation work

1. Add generic arrays of color attachments, optional depth, load/store/clear descriptions, matching pipeline color formats, independent depth-test/write/compare and blend/write-mask state.
2. Implement six-color-plus-depth dynamic rendering on Vulkan and Metal attachment descriptors with persistent depth stores/loads; preserve existing one-image convenience overloads.
3. Add needed scalar/vector/integer formats and format/usage/attachment limits. Expose primitive capability queries in gpu, distinguishing query RT from full tracing support.
4. Validate incompatible counts/formats/usages before recording. Keep schema names such as albedo, transparent layer or G-buffer out of primitive GPU APIs.

## Verification and exit conditions

- A primitive MRT probe writes distinct channels and reuses depth on both backends, without scene/render dependencies.
- Existing triangle/cubemap examples retain behavior; unsupported attachment configurations return a clear capability/validation result.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M01-10/M01-11 use these primitives; M03/M04/M05 must reuse or deliberately extend them rather than add native Vulkan code elsewhere.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## Implemented interface reference

See [GPU rendering primitives](gpu-rendering-primitives.md) for actual descriptions, capability queries, validation limits and the manual MRT probe. Acceptance is recorded in progress.md.
