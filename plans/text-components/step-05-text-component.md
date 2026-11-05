# Step 5 — TextComponent and procedural drawable lifecycle

Dependency: accepted step 4.

## Implementation

1. Add virtual read-only `DrawableComponent::requiresBg2Export() const`, default true. Override it in `TextComponent` to return false. Retain `isSceneGeometry() == true`; this is authored scene geometry, not an overlay.
2. Implement `TextComponent : DrawableComponent` holding its Text data, independent procedural generator/source settings and exactly one quad/submesh/material. Hide irrelevant construction conveniences in its direct API without claiming safety against base-pointer/mutable API modifications.
3. Generate consistent normals, tangents, winding and UV sets. Define quad size, paragraph aspect and anchor/pivot. Changing bitmap resolution alone must not change world size or line breaks.
4. Connect the resolved scene font and generator through ordinary base::Texture and draw::Drawable loading. Set sourceEditable false and ignoreSerialization true on generated albedo. Use a reasonable default alpha-test cutoff and doubleSided true for newly authored text; leave appearance fully editable through the material, including unlit.
5. Preserve color, all ordinary material values, UV set/scale and sampling controls when regenerating or applying a complete material. Font/layout changes must not reset these settings. Do not detect-and-repair arbitrary source replacement every frame.
6. Detect text/font revision changes and prepare/publish resources at the engine's coordinated mutation boundary, with background producers stopped as required. Do not upload/reload arbitrarily during scene traversal or destroy resources still referenced by submitted work. Coalesce changes; leave unchanged frames upload-free and avoid redoing geometry for pixel-only edits.
7. Implement lifecycle hooks, factory registration, module includes and independent detached clone behavior. Handle unrealized/loaded clones, scene replacement, missing fonts, load failure and scene teardown safely. Reuse existing ownership/retirement patterns.
8. Audit render extraction, selection/gizmos and picking for subclass support. Picking uses all six quad indices and does not discard by glyph alpha; backface selection follows doubleSided. Do not redesign multi-drawable nodes.
9. Audit glyph alpha in visibility/shadow paths as well as raster display. Text must cast glyph-shaped, not rectangular, shadows, including the directional-light path used by the final tutorial. If a missing shared alpha-test path must be implemented, do so here for the relevant backends rather than hiding the problem in tutorial-only code. Normal material semantics remain consistent.

Do not add a temporary example or Composer UI here. Embedded persistence is completed in step 6 before exposing authoring in step 7.

## User handoff: Composer

Report the new component/API, ownership and default material values. New authoring controls are not yet available, so successful user compilation of library and Composer is sufficient acceptance. State that integrated appearance, shadow and picking observations will be available in step 7; do not claim those observations have been performed.

Update progress and stop.

## Next milestone

Step 6 implements embedded material persistence and self-contained scene resources without generating text `.bg2` files or albedo PNGs.
