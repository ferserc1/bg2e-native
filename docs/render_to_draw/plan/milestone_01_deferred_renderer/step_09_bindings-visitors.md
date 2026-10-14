# M01-09 — Draw data bindings, visitors and queues

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M01-06–M01-08.

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

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M01-10/M01-11 consume these APIs; M02 picking and M03 material tables must preserve identical scene/submesh identity and transform conventions.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
