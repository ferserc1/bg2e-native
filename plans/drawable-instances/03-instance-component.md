# Step 03: Implement instance state and integrate rendering

## Prerequisite

Step 02 implemented and buildable; record its user validation status.

## Work

Add DrawableInstanceComponent directly under DrawableRenderComponent and register its factory/reflection. Add Node::drawableInstance() as a fast concrete accessor. Do not broaden Node::drawable().
Instances hold a stable resource handle and independent material realizations, transforms, names, group names, visibility and applicable per-submesh properties copied from the original Drawable at creation. Clone shares geometry but deep-copies independent state. Do not expose mesh mutation through the instance API; const Drawable alone is insufficient because existing mesh() exposes mutable geometry.
Reconcile topology by index: truncate excess state; append defaults from corresponding original submeshes; emit warnings only. Render queues accept shared geometry plus effective component state. Integrate bounds, picking, selection, overlays and all generic visitors; inspect direct DrawableComponent casts.
Share local-space BLAS as well as raster buffers. Supply each effective world/submesh matrix and material to TLAS extraction. Keep old vertex/index buffers alive with in-flight BLAS snapshots. No new per-instance BLAS or GPU API redesign.

## Buildable completion boundary

All new declarations, definitions and affected consumers are integrated. Run the build policy in summary.md; record outcomes in progress.md. Keep optional probes simple. Do not mark user validation complete yourself.

## Composer validation

No dedicated controls yet; existing geometry/text must remain unaffected.

## Handoff

Report changes, build result and applicable user checks in Spanish. Update progress.md. Next step: 04-ownership-lifecycle.md
