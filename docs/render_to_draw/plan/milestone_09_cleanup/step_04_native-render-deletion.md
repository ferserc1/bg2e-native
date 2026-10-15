# M09-04 — Delete render implementation and confine native graphics to GPU

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M09-01–M09-03.

## Scope and deliverable

- `lib/include/bg2e/render/`
- `lib/src/bg2e/render/`
- `lib/src/bg2e/ui/ImGuiVulkanBackend.cpp`
- `lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp`
- `lib/include/bg2e/gpu/vk/`
- `lib/include/bg2e/gpu/metal/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/](../../../../lib/include/bg2e/render), [lib/src/bg2e/render/](../../../../lib/src/bg2e/render), [lib/src/bg2e/ui/ImGuiVulkanBackend.cpp](../../../../lib/src/bg2e/ui/ImGuiVulkanBackend.cpp), [lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp](../../../../lib/src/bg2e/app/detail/DrawGraphicsExecution.cpp), [lib/include/bg2e/gpu/vk/](../../../../lib/include/bg2e/gpu/vk), [lib/include/bg2e/gpu/metal/](../../../../lib/include/bg2e/gpu/metal).

## Implementation work

1. Delete the production render tree and its specialized ViewpointAnalyzer/BlueNoise/bake/forward implementations after confirming replacements; no hidden archived runtime code or forwarding namespace remains.
2. Audit remaining native Vulkan/VMA/SDL-Vulkan commands and wrappers outside GPU. Move necessary native UI, SDK and drawable-size/window graphics operations into GPU backend implementation/bridges; high-level coordination stays in its namespace.
3. Keep reflection/JSON/material/scene/gpu implementations independent of the deleted tree. Do not introduce GPU dependencies on ui/scene/draw to move native code: backend bridges accept primitive/native data or callbacks.
4. Separate actual native API implementation from explicit ShaderBinding.vulkan indices, backend type/handler casts and GLSL syntax, which remain legitimate. Native ImGui headers/types still do not enter public engine headers.

## Verification and exit conditions

- Supported C/C++ headers/sources have no render/legacy bindings references and no new native Vulkan implementation outside GPU.
- Both GPU backends, FSR bridge and UI composition function with their final interfaces; a Vulkan scan does not incorrectly delete required GPU SDK/MoltenVK assets.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M09-05 removes obsolete shader/build/package/test paths; M10 documents backend-native seams and the final public extension surface.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
