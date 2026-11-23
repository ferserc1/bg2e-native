# DrawableInstanceComponent

Header: `<bg2e/scene/DrawableInstanceComponent.hpp>`; namespace `bg2e::scene`.
Factory/JSON type: `DrawableInstance`. Direct base: DrawableRenderComponent.

## Binding and appearance

`setResource(DrawableCache&, const DrawableResourceHandle&)` returns a
DrawableCacheDiagnostic. It binds resident shared geometry and initializes
independent appearance from immutable originals. `resourceHandle()` and
`resourcePath()` identify the binding; `clearResource()` releases it. Keep
Engine/Device alive and stop producers for binding/resource operations.

`material(index)` exposes the authored CPU definition; `setMaterial()` deep-copies
settings. `effectiveMaterial()` and `drawMaterial()` inspect independent CPU/native
appearance. Set model-local submesh matrices, names, groups, visibility and
tracing through `setSubmeshTransform`, `setSubmeshName`,
`setSubmeshGroupName`, `setSubmeshVisibility` and `setRayTracingEnabled`.
`submeshTransform()` returns the local matrix. The node's
`TransformComponent` controls placement. Nonfinite,
singular or nonaffine matrices warn and preserve the previous value.

These operations never edit raster/BLAS storage. `clone()` keeps the resource
identity and deep-copies appearance. Geometry revisions reconcile by index:
truncate excess, fill missing entries from originals and warn. Composer owner
replacement retains exact before/after appearance for undo/redo.

## Preparation and recovery

`prepareResources(engine)` and its subtree overload prepare at a stopped boundary.
Their bool reports change, not universal success; inspect `resourcesLoaded()`.
`retryResources()`, `releaseResources()` and `reportRenderFailure()` support
explicit recovery/teardown. Engine shutdown invokes the cache's weak payload
release hooks before Device destruction. Dependency issues expose unresolved
binding/material state while retaining references and authored properties.

Identity-validated material commands that update both CPU/native state call
`acknowledgeMaterialEdit(index)` so future preparation retains native identities.
Standalone users must preserve the same synchronization and lifetime rules.

## Persistence and Composer creation

For the complete JSON schema and SDK import rules, see the
[drawable scene format](../db/DrawableSceneFormat.md).

Reference JSON stores `path`, `rayTracingEnabled` and ordered
`submeshes` objects containing `material`, `transform`, `name`, `groupName` and
`visible`. Material fields use conventional PBR texture/sampling/indirect metadata.
Missing/excess properties recover against originals; unresolved references remain
recoverable. Complete scene export packages independent images and coordinates
one model export per identity. Reference-only IO exports no pixels.

Composer creation from an owner copies its current live appearance, including
unsaved materials/lightmaps and submesh properties, into a detached candidate.
Creation from the cache uses originals. Both share geometry/BLAS; undo/redo
retains the captured independent candidate. See [Composer](../../apps/draw_composer.md#drawable-instances).
