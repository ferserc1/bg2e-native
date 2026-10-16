# Scene manipulation API

**Namespace:** `bg2e::manipulation`

The manipulation module provides scene selection and viewport tools. Its
selection and gizmo types have separate production (`bg2e::render`) and
experimental (`bg2e::draw`) execution modes. Construct them with the engine
that owns the target scene; do not mix resources between modes.

## Main types

| Type | Header | Purpose |
|---|---|---|
| [`SelectionManager`](reference.md#selectionmanager) | `<bg2e/manipulation/SelectionManager.hpp>` | Picking, typed submesh selection, selection queries and transform-input capture. |
| [`GizmoComponent`](reference.md#gizmocomponent) | `<bg2e/manipulation/GizmoComponent.hpp>` | Runtime-only node component for camera/light gizmos and transform handles. |
| `SelectableComponent` | `<bg2e/manipulation/SelectableComponent.hpp>` | Persistent scene identity used to validate selectable objects. |
| `GizmoAndSelectionRenderer` | `<bg2e/manipulation/GizmoAndSelectionRenderer.hpp>` | Renderer overlay that draws selection and gizmos for the matching engine mode. |

`SelectionItem` stores weak references to its node, drawable component and mesh,
plus submesh and execution-mode identity. Call `refreshSelection()` at scene
update boundaries after removals or resource replacement to prune stale items.
Gizmos are transient and are not serialized. Release draw gizmo resources while
the associated engine/device is still alive.

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
