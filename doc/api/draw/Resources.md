# Draw resources, scene references and bindings

Headers: `<bg2e/draw/Drawable.hpp>`, `<bg2e/draw/MaterialBase.hpp>`,
`<bg2e/draw/Texture.hpp>`, `<bg2e/scene/DrawableComponent.hpp>`.

`draw::Drawable` connects CPU `geo::MeshPNUUT` geometry/submeshes to GPU
`drawMesh()`, `drawMaterial(index)` and optional tracing geometry. `setMesh`
establishes submesh attributes; `load(Engine*)` realizes resources. Set materials,
names, groups, transforms and visibility per submesh. `material(index)` addresses
live attributes once loaded; use `updateMaterials()` when resource realization
must change. Scalar live attributes and texture/resource replacement have distinct
costs; do not rebuild textures every frame.

`clone()` duplicates CPU geometry, GPU mesh/material realizations and mutable
texture/image/sampler ownership, including built acceleration resources. It is
not default instancing. The Engine/device is borrowed and must outlive both copies.
`reload`, mesh/texture edits, cloning loaded resources and cleanup are rare-edit
operations: stop producers, synchronize outstanding users, mutate and resume.
`utils::TextureCache` keeps weak draw realizations with engine/device validation;
explicit cached sharing does not override clone independence.

`scene::DrawableComponent::drawable()` returns the sole draw realization; const
component access returns a const Drawable. `setDrawable`/`clearDrawable` release
component references at a safe boundary, checking context ownership on replacement.
Schema source references can exist without a loaded Drawable. Graph cloning uses
deep resource cloning; native scene Drawable types and mode tags are removed.

`draw::DrawVisitor`, `RenderQueueVisitor` and `CollectRayTracingInstancesVisitor`
consume the new geometry. Bindings live under draw: `FrameDataBinding`,
`ObjectDataBinding`, `LightDataBinding`, `EnvironmentDataBinding`. They retain
slot-safe mutable shader data and matching GPU resource snapshots. Canonical
queue object IDs are one-based 24-bit snapshot-local identities, zero background;
node/submesh identity is distinct from transparent sorting and dense query order.
These IDs are not persistent editor selection IDs.

CPU materials retain two UV sets (UV0/UV1), packed-channel selection, texture
scales and inversion. [Baked indirect maps](BakedIndirectMaps.md) explicitly
interpret the AO slot as scalar AO or linear RGB baked AO/GI on UV2. GPU-only
source settings are not an exported image; preserve live/stored material modes
and materialize image data before saving a reloadable resource. The [deferred example](../../tutorials/draw/10_deferred_renderer.md)
contains clone, UV1 and packed-channel probes.

See [database representation/resource IO](../db/SceneReferences.md).
