# M01-09 — Draw data bindings, visitors and queues

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M01-06–M01-08.

## Scope and deliverable

- `lib/include/bg2e/scene/vk/FrameDataBinding.hpp`
- `lib/include/bg2e/scene/vk/ObjectDataBinding.hpp`
- `lib/include/bg2e/scene/DrawVisitor.hpp`
- `lib/include/bg2e/scene/RenderQueueVisitor.hpp`
- `lib/include/bg2e/scene/ResizeViewportVisitor.hpp`
- `lib/include/bg2e/draw/RenderQueue.hpp`
- `lib/include/bg2e/draw/DrawVisitor.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/scene/vk/FrameDataBinding.hpp](../../../../lib/include/bg2e/scene/vk/FrameDataBinding.hpp), [lib/include/bg2e/scene/vk/ObjectDataBinding.hpp](../../../../lib/include/bg2e/scene/vk/ObjectDataBinding.hpp), [lib/include/bg2e/scene/DrawVisitor.hpp](../../../../lib/include/bg2e/scene/DrawVisitor.hpp), [lib/include/bg2e/scene/RenderQueueVisitor.hpp](../../../../lib/include/bg2e/scene/RenderQueueVisitor.hpp), [lib/include/bg2e/scene/ResizeViewportVisitor.hpp](../../../../lib/include/bg2e/scene/ResizeViewportVisitor.hpp).

## Implementation work

1. Implement frame/object/material/light/environment bindings under draw using GPU resources and explicit Vulkan/Metal binding indices; preserve actual six-texture material packing.
2. Put new DrawVisitor, RenderQueueVisitor and CollectRayTracingInstancesVisitor in draw. Keep legacy visitors/bindings available only for production until M09.
3. Retain UpdateVisitor/InputVisitor/TransformVisitor in scene. Remove the unnecessary Engine/native extent dependency from ResizeViewportVisitor if straightforward; otherwise keep the rendering-specific adapter in draw.
4. Preserve nested transform multiplication, submesh visibility and opaque/transparent/solid-transparent classification/sort. Use slot-safe mutable shader data, not rings of geometry/textures.

## Verification and exit conditions

- Nested scene transforms and disabled/hidden submeshes agree in raster and query extraction; material bindings match shader ABI on both backends.
- No new scene::vk implementation or native drawing callbacks are added to scene. Unsupported mesh layout behavior is explicit.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M01-10/M01-11 consume these APIs; M02 picking and M03 material tables must preserve identical scene/submesh identity and transform conventions.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-06 resource handoff

Use `DrawableComponent::executionMode/drawDrawable`, `draw::Drawable::drawMesh/materials`, submesh transforms/visibility and the material/texture accessors from M01-06. Do not route draw scenes through production RenderQueueVisitor. Bind explicit image/sampler resource sets per frame as needed; geometry and texture realizations are not rings. Check same-Engine/device context and rebind views after texture or geometry replacement. Material flags/channels/UV selections and colorType are preserved CPU state for the shader ABI, not an automatic binding/decoding rule. See [drawable-resources.md](drawable-resources.md).

## M01-08 environment handoff

EnvironmentComponent now exposes pure source/normalized skyboxBlur/skyboxVisible intent. Extract that intent without native descriptors; generated shared EnvironmentResources belong to draw::environment. Resource replacement must run at a stopped-producer boundary before recording, then rebind lighting/skybox sets to the new snapshot. See [environment-tools.md](environment-tools.md).

## Implemented M01-09 handoff

See [bindings-visitors.md](bindings-visitors.md) for actual queue/visitor identities, slot-safe binding lifecycle, packed structs, explicit Vulkan/Metal indices and lead probes. The new code is draw-only; production bindings/visitors remain available. ResizeViewportVisitor now accepts structural extents without Engine/native includes.
