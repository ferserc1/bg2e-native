# M09-03 — Remove legacy shared overloads, scene resources and bindings

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M09-01/M09-02.

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

Preserve [M07's neutral contracts](../milestone_07_bg2e_composer/review.md):
Scene::releaseSceneRoot, subclass-compatible lookup, rejection of sliced/shared
clones, independent projections, parsed-model CPU/read/draw realization APIs,
chain metadata and isSceneGeometry helper exclusion. Keep all surviving
SelectableList item/endList calls under a successful bool beginList scope.
Removing production overloads must not remove those shared behavior fixes.

1. Delete legacy scene Drawable/RenderMesh and native draw methods; DrawableComponent now holds draw Drawable only, with consistent final accessors used by all supported consumers.
2. Remove production Engine/material/texture overloads and backing maps from DB/scene factories/utils/manipulation/UI/bake wrappers. Retain useful pure serialization/CPU overloads.
3. Delete scene::vk and legacy DrawVisitor/RenderQueueVisitor implementations; keep draw visitors and scene Update/Input/Transform visitors. Finalize ResizeViewportVisitor based on actual neutral dependency.
4. Remove temporary component mode tags/adapters/output aliases and obsolete concrete widget variants. Preserve independent GPU clones, MaterialModifier patch behavior and simple TextureCache purpose.

## Verification and exit conditions

- No shared public API mentions production types or silently falls back to production; graphics-free serialization/mesh operations still work.
- Scene/component/manipulation/UI/bake callers are updated together, with stable scene/submesh/resource behavior after removing dual paths.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M09-06 checks final behavior. M10 references final namespaces/accessors, not transitional drawDrawable/legacy overload instructions.

The [M02 review](../milestone_02_manipulation/review.md) records the concrete
draw replacements: typed weak selection identities, draw MaterialModifier
overloads, separate queued draw widgets, UI-session texture previews, optional
overlays and the `utils::ViewpointAnalyzer(draw::Engine*)` path. Delete each
production overload only after all corresponding model_edit/composer/remaining
production callers and the M02/M06/M07 behavior gates are accounted for. Do not
collapse CPU definitions and live draw material targets into a whole-material
write during API cleanup.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.


## M04-07 reviewed implementation requirement

Retain bake::UvMapPreviewRenderer (CPU diagnostic plus generic image upload) and ui::BakeLightmapPreview as canonical draw-compatible replacements. Delete the production render::UvMapPreviewRenderer/ui::UvMapPreview coupling only after actual editor callers have migrated. Preserve explicit IndirectTextureMode metadata, linear UV2 sampling and live/stored material consistency while removing legacy overloads.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).

M06 adds a concrete application caller for draw material editing, typed texture
preview, selection/gizmo overlays, viewpoint analysis, complete draw IO and
integrated bake. The review records their current headers and ownership. At
retirement, migrate all production `model_edit`/composer callers before deleting
their overloads, preserve native resource format readability, and keep app dirty
state/generations out of shared widgets and DB. M06 continuation does not supply
runtime regression evidence; M09-06 must collect it before claiming equivalence.
See the [M06 review](../milestone_06_model_edit/review.md).
