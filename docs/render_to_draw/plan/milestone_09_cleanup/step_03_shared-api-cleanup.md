# M09-03 — Remove legacy shared overloads, scene resources and bindings

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M09-01/M09-02.

## Scope and deliverable

- `lib/include/bg2e/scene/Drawable.hpp`
- `lib/src/bg2e/scene/Drawable.cpp`
- `lib/include/bg2e/scene/DrawableComponent.hpp`
- `lib/include/bg2e/scene/Mesh.hpp`
- `lib/include/bg2e/scene/vk/`
- `lib/include/bg2e/utils/TextureCache.hpp`
- `lib/include/bg2e/utils/MaterialModifier.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/scene/Drawable.hpp](../../../../lib/include/bg2e/scene/Drawable.hpp), [lib/src/bg2e/scene/Drawable.cpp](../../../../lib/src/bg2e/scene/Drawable.cpp), [lib/include/bg2e/scene/DrawableComponent.hpp](../../../../lib/include/bg2e/scene/DrawableComponent.hpp), [lib/include/bg2e/scene/Mesh.hpp](../../../../lib/include/bg2e/scene/Mesh.hpp), [lib/include/bg2e/scene/vk/](../../../../lib/include/bg2e/scene/vk), [lib/include/bg2e/utils/TextureCache.hpp](../../../../lib/include/bg2e/utils/TextureCache.hpp), [lib/include/bg2e/utils/MaterialModifier.hpp](../../../../lib/include/bg2e/utils/MaterialModifier.hpp).

## Implementation work

1. Delete legacy scene Drawable/RenderMesh and native draw methods; DrawableComponent now holds draw Drawable only, with consistent final accessors used by all supported consumers.
2. Remove production Engine/material/texture overloads and backing maps from DB/scene factories/utils/manipulation/UI/bake wrappers. Retain useful pure serialization/CPU overloads.
3. Delete scene::vk and legacy DrawVisitor/RenderQueueVisitor implementations; keep draw visitors and scene Update/Input/Transform visitors. Finalize ResizeViewportVisitor based on actual neutral dependency.
4. Remove temporary component mode tags/adapters/output aliases and obsolete concrete widget variants. Preserve independent GPU clones, MaterialModifier patch behavior and simple TextureCache purpose.

## Verification and exit conditions

- No shared public API mentions production types or silently falls back to production; graphics-free serialization/mesh operations still work.
- Scene/component/manipulation/UI/bake callers are updated together, with stable scene/submesh/resource behavior after removing dual paths.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M09-06 checks final behavior. M10 references final namespaces/accessors, not transitional drawDrawable/legacy overload instructions.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
