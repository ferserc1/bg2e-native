# Drawable caches and instances

## Ownership and rendering

The draw resource cache requires an initialized `draw::Engine` and its Device.
It has no Composer, scene-tree or UI dependency. `Engine::drawableCache()` provides
the default context; `Engine::createDrawableCache()` creates a separate shared
context for a document or job. Producers must be stopped for resource mutations.

Three sibling components derive directly from `scene::DrawableRenderComponent`:

| Component | Geometry and appearance |
|---|---|
| `DrawableComponent` | Exclusive authored owner of a cached Drawable identity. |
| `DrawableInstanceComponent` | Shared geometry identity; independent appearance and placement. |
| `TextComponent` | Privately owned procedural text geometry/material; no model ownership. |

`Node::drawable()` keeps its concrete owner meaning. Use `drawableInstance()` for
an instance or `drawableRender()` for common rendering, bounds, picking and
material inspection. Instances do not expose mutable mesh storage. A const
Drawable alone is insufficient for that restriction because its legacy mesh
accessor exposes mutable storage. Text is excluded from BG2 model export.

One resource permits at most one SceneOwner and many SceneInstance registrations.
An instance shares local raster buffers and BLAS with its resource. Node/world and
independent drawable/submesh matrices supply each TLAS placement. Effective
materials belong to each instance. This reduces geometry residency, not draw calls;
there is no new batching or GPU abstraction contract.

## Identity, originals and independent state

BG2/VWGLB lookup uses normalized full source paths. Procedural/imported resources
receive unique identities; imported labels include the source filename. Display
names, lookup aliases and export destinations are distinct. Save As does not turn
an export destination into an implicit source alias.

Cache entries retain immutable original submesh definitions and drawable transform.
`DrawableInstanceComponent::setResource()` initializes independent CPU material
settings, local transforms, names, groups and visibility from these originals.
Each instance realizes its own native materials and retains tracing eligibility.
Changing an instance does not edit the owner, originals or another instance.

Composer provides two creation policies:

- From the cache browser, use the resource originals.
- From a selected owner, bind the same identity and copy the owner's current live
  materials, texture settings, drawable/local transforms, names, groups,
  visibility and tracing state. Unsaved changes and generated lightmaps are copied
  without saving or rereading the BG2. Texture source lifetime is retained while
  mutable settings and materials are independent.

Owner-based creation prepares the detached candidate before history admission.
Invalid/unavailable source state rejects insertion. Undo/redo keeps that captured
candidate, rather than recopying whatever appearance the owner has later.
Instance duplication shares geometry but deep-copies independent state. Owner
and legacy duplicate-owner copies retain distinct identities/model destinations.

## Replacement and topology

`DrawableComponent::setDrawable()` replaces geometry within its current identity;
`rebindResource()` explicitly chooses another identity. Ownership conflicts,
foreign resources and incompatible devices reject locally. Replacement publishes
only complete geometry/native resources and advances geometry revision. Instances
follow the identity without becoming mutable geometry owners.

Appearance is reconciled by submesh index: preserve common indices, truncate
excess state and append missing values from the corresponding originals. Warnings
explain mismatched topology. Composer replacement prepares before/after instance
appearances and retained geometry/default versions so undo restores both the old
native realization and the correct originals for future instances.

## Residency, history, jobs and frames

The cache strongly retains zero-use entries until explicit unload/clear or context
retirement. A copied `DrawableResourceHandle` is nonowning and registers no use.
Use the move-only `DrawableUsageLease` for logical SceneOwner, SceneInstance,
History, Job and Frame retention. History/jobs/frames pin their acquired
realization. Frame retention must last through GPU completion, not just recording.

`canUnload()`/`unload()` inspect logical uses, not shared-pointer counts.
`clear()` refuses transactionally while any entry is used. Browser DTOs retain
neither native resources nor owning cache references merely by being displayed.
Material commands retain independent definitions/textures and before/after disk
paths. Generic subtree retention aggregates appearances even when resource
identity is deduplicated. Shared geometry/BLAS should be charged once per resource;
independent materials and source owners still need their own accounting.

Node document scopes are inherited. True New/Open constructs a fresh scope; old
history/jobs/frames may retain an old scope until explicitly retired. Composer
stops old bake jobs after successful replacement, while failed publication keeps
the old document. Engine shutdown invalidates handles, invokes registered weak
payload release hooks and releases native realizations before destroying Device.
Keep separately extracted native references within that lifetime too.

## Scene representation and coordinated export

Instances remain ordinary entries in Node `components` arrays with type
`DrawableInstance`. They store a model `path`, independent `transform`,
`rayTracingEnabled` and ordered `submeshes`, each with `material`, `transform`,
`name`, `groupName` and `visible`. There is no persisted top-level cache array.
Unused resident entries, runtime handles, usage counts and cleanup roots are not
serialized. Reference-only saving copies no assets and can require a prior complete
export for procedural/GPU-only sources.

A complete save builds one operation-local resource table from nonignored owners
and instances. It stages one BG2 per used identity: owner properties if an owner is
present, immutable originals otherwise. Instances never independently export or
rewrite shared geometry. Their material images are packaged separately in scene
JSON. Published file geometry may bake a submesh basis; the coordinator rebases
instance transforms so placement is preserved without mutating resident geometry.

Ordinary intended model saves overwrite their destination through staged
publication and rollback. Independent collision copies reserve distinct files and
cannot overwrite an unrelated model. BG2 staging snapshots read existing staged
images; serialized instance references name final published images. Live authored
paths are restored after save, and the main JSON is published last.

Recoverable export warnings may accompany a written JSON; inspect the save report
before marking an editor clean. Missing resources keep recoverable references and
prior complete files where possible. Complete failure does not fabricate writes.
Legacy repeated owner paths become independent owners/files, with diagnostics;
instances continue sharing the first cached identity. Raw document IO remains the
way to preserve unknown JSON; typed saves cannot restore skipped component data.

## Cleanup protection and lifecycle

`db::SceneResourceManager` still supports device-free file-only analysis. Runtime
callers supply `SceneResourceProtection`: file/model roots plus cache identity and
content-generation values. Composer collects reachable document contexts and
resident originals, current and retained geometry/material versions, independent
instance lightmaps, history/job/frame paths and clipboard sources.

Analysis expands known model dependencies and records the local inventory and
file fingerprints. A resource load, unload or other content mutation invalidates
review; ordinary lease turnover does not advance cache generation. Warning
acknowledgement, final delete confirmation and stale-plan rejection remain
explicit. External files and symlinks are never deletion candidates.

Cleanup transactionally reloads the saved scene before deletion. This internal
reload transfers all reviewed cache scopes to the candidate root, preserving
cache-only entries. Rebinding a cleanup plan requires unchanged reviewed cache
generations and filesystem snapshot, no new warnings and no expansion of the
confirmed deletion list. Reviewed history/clipboard roots remain protected for
that operation even when those consumers retire. New/Open does not transfer
these scopes; a true reopen drops unused nonpersisted protection.

`DrawableRegistry` supplies separate drawable-name bookkeeping. It is neither the
resident library nor disk inventory; its register path is not the authority used
by this implementation. SceneResourceManager never clears the resident library
as a side effect of deleting disk orphans.

## Bake, offscreen and boundaries

Integrated and standalone contexts accept instance receivers with independent
materials and transforms. Composer snapshots registered owners as instances to
share raster/BLAS while freezing appearance. A later shared geometry replacement
invalidates bake source checks; retained versions remain alive through retirement.
Lightmaps use each instance's serialized AO-slot bakedAO/bakedGI material state.
UV2 generation modifies owner geometry and is unavailable on instances. Standalone
instance receivers require device-backed loading and existing UV2; image/metadata
output identities remain per node. Optional BG2 output copies remain owner-only.

The existing offscreen application accepts `--cached-model FILE`: it loads a BG2
twice before node binding, checks identity reuse, retains a Job use and renders two
independent appearances. Cache services require the initialized backend even in
windowless jobs. There is no HTTP server implementation in this feature, no
device-free drawable cache, and no promise of persistent unused library entries.
Future cache persistence must preserve the identity/export separation rather than
silently writing unused resources into current scene files.

See [DrawableCache](../api/draw/DrawableCache.md),
[renderable components](../api/scene/DrawableRenderComponent.md),
[scene IO](../api/db/SceneReferences.md),
[complete export](../api/db/DrawResourceIO.md),
[cleanup](../api/db/SceneResourceManager.md),
[Composer](../apps/draw_composer.md#drawable-instances) and
[retained commands](command-memento/composer-resources.md).
