# Phase 02 Material revisions and raster bindings

## Outcome

Unchanged materials reuse packed shader data, and unchanged raster object/environment texture bindings are not reassigned each frame. Each acquired slot still receives all changes it has not consumed.

## Implementation

1. Add monotonic revisions for material values, realized image/sampler bindings, and classification changes. Setters that receive an unchanged effective value should not increment the relevant revision. Authored edits below the alpha threshold still update authored state even when the effective GPU state is unchanged.
2. Cover mutations through `materialAttributes()` references, whole-attribute assignment, `Drawable::material()`, cloning, texture realization, and sampler replacement. Track at the mutation source or introduce a compatible edit mechanism covering all callers. A flag only in `MaterialBase::setMaterialAttributes()` is insufficient. Keep existing pending-texture-realization flags distinct from renderer cache revisions.
3. Account for resource settings changed through referenced texture objects. Use resource identity/generation checks or a revision propagated by the existing realization path; pointer identity alone is insufficient if native state can change in place.
4. Cache `packMaterial()` output by material identity and consumed revisions. Validate newly changed state before publishing it. Preserve source identities for clones and independent materials with equal values.
5. In `ObjectDataBinding`, track each record's source identity, uniform contents, and texture/sampler generations. Update transforms or values when needed, and skip binding setters and `update()` when bindings are unchanged. Queue reorder or record replacement must invalidate the record correctly.
6. Apply equivalent binding-change detection to `EnvironmentDataBinding`, separating uniform changes such as sky blur from image/sampler changes.

Use revisions consumed independently by each frame slot and cache; never clear one shared dirty flag after the first consumer. If revision tracking touches shared base classes, preserve existing serialization and behavior outside the requested optimization.

## Verification and exit

Exercise each mutation route, delayed slot reuse, multiple consumers, unchanged assignments, clone independence, asynchronous texture completion where supported, environment changes, and queue reorder. Validate that invalid edited state cannot expose partially rebound resources.

After warmup, unchanged material pack counts and raster descriptor update counts should approach zero for unchanged records. Moving an object should update its transform without rebinding its textures. Exit with those reductions active in the functional renderer; revisions must already serve these consumers rather than exist only for later phases.
