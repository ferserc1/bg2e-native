# Scene manipulation API

**Namespace:** `bg2e::manipulation`

The manipulation module provides draw scene selection and viewport tools.
Construct tools with the draw Engine owning the target resources; native production
overloads, PickSelectionVisitor and temporary mode discriminators are removed.
PickKind remains a neutral public enum in PickKind.hpp.

## Main types

| Type | Header | Purpose |
|---|---|---|
| [`SelectionManager`](reference.md#selectionmanager) | `<bg2e/manipulation/SelectionManager.hpp>` | Picking, typed submesh selection, selection queries and transform-input capture. |
| [`GizmoComponent`](reference.md#gizmocomponent) | `<bg2e/manipulation/GizmoComponent.hpp>` | Runtime-only node component for camera/light gizmos and transform handles. |
| `SelectableComponent` | `<bg2e/manipulation/SelectableComponent.hpp>` | Persistent scene identity used to validate selectable objects. |
| `GizmoAndSelectionRenderer` | `<bg2e/manipulation/GizmoAndSelectionRenderer.hpp>` | Renderer overlay that draws selection and gizmos for the owning draw engine. |

`SelectionItem` stores weak references to its node, drawable component and mesh,
plus submesh and selection identifier. Call `refreshSelection()` at scene
update boundaries after removals or resource replacement to prune stale items.
Gizmos are transient and are not serialized. Release draw gizmo resources while
the associated engine/device is still alive.

Applications that supervise gizmo drags with history should capture the initial
transform and adopt the final preview at the graphics boundary. The optional
Composer lifecycle and its conflict rules are documented in the
[continuous interaction](../../architecture/command-memento/continuous-interactions.md)
and [lifecycle](../../architecture/command-memento/origins-and-lifecycle.md) chapters.

## Picking and input

The draw picker uses top-left viewport coordinates and an explicit drawable-pixel
scale. It renders visible geometry IDs and reads the selected pixel back
synchronously, so call it at a stopped scene/GPU boundary, outside frame
recording. Picking is geometric: material alpha/cutout is not tested. See the
[picking reference](reference.md#draw-picking) for scale, identity and lifecycle
details.

For a complete event-routing and selection example, see the
[manipulation tutorial](../../tutorials/draw/11_manipulation.md). The renderer
overlay API is also described in [`GizmoAndSelectionRenderer`](reference.md#overlay).
