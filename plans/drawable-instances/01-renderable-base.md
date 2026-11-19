# Step 01: Introduce the common renderable contract

## Prerequisite

Plan approved; implementation not yet started.

## Work

Create scene::DrawableRenderComponent as a Component-derived read contract for geometry, effective submesh materials, transforms, visibility and tracing eligibility. Keep model paths, cache ownership and geometry mutation out of this contract.
Move DrawableComponent and TextComponent directly under this base. Text keeps its privately owned generated Drawable, resource preparation, quad picking, paragraph/material serialization, recovery and deep clone behavior. Do not register generated text geometry as library models.
Keep Node::drawable() unchanged in meaning; add Node::drawableInstance() when its concrete type is introduced in step 03. Adapt existing generic renderable consumers to the base without changing existing output. Audit Text casts, reflection, compatibility checks and component commands: Text must not silently disappear when it ceases to derive from DrawableComponent.
Preserve source compatibility where possible; these inheritance changes are explicitly within this approved plan.

## Buildable completion boundary

All new declarations, definitions and affected consumers are integrated. Run the build policy in summary.md; record outcomes in progress.md. Keep optional probes simple. Do not mark user validation complete yourself.

## Composer validation

Existing ordinary geometry and text remain visible/selectable. No new Composer controls yet.

## Handoff

Report changes, build result and applicable user checks in Spanish. Update progress.md. Next step: 02-cache.md
