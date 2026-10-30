# Composer resources

## Materials and textures

`EditMaterials` retains authored before/after definitions and exchanges values
on the same live `MaterialBase`, preserving submesh identity. Shared targets are
deduplicated. Undo/redo reuses retained definitions and texture realizations;
it does not re-read a path. Ambiguous shared live materials, custom material
types and unrealized sources reject. Renderer material packing is validated
before publication.

`Texture::retainSource` carries a source owner and declared payload bytes through
copies, material settings, snapshots and clipboard clones. Generated bake
outputs use a shared temporary-directory lease; the directory is removed when
the last owner releases it. This does not change global `TextureCache` policy.

## Environment and GPU-derived state

Environment commands snapshot authored component values and canonical generator
state. Replay prepares resources on the current compatible device and exchanges
authored state while preserving component identity. Same-device rebind is
allowed; incompatible resources reject. Inactive environment edits preserve the
active publication. General structural edits to file-backed environments remain
protected; prepared presets have a dedicated retained adapter.

GPU realizations are derived and may be rebuilt during undo/redo. Preparation
occurs while publication is stopped; resource retirement waits for the
application's GPU drain. Native acceleration-backed geometry/structure has no
retention adapter and rejects.

## Import, UV2 and lightmap adoption

Parsing/baking/generation are not commands. Their completed results are
prepared privately, then adopted: import stores a stable reimport mapping slot
and old/new graph placement; UV2 uses `ReplaceGeometry`; lightmap prepares every
file, texture and material definition before one batch command. Temp image source
leases keep generated files alive across cleanup, undo and redo. The original
job token/source generation guards only initial adoption. Replay remains valid
after the job has completed, while document/target generation and membership
remain checked.

Import validation records size and modification time for the primary file, not
an atomic snapshot/hash of all referenced dependencies. Redo uses captured
prepared scene data rather than reloading. If preparation or admission fails,
there is no partial authored publication. GPU/device incompatibility and
unsupported custom payloads reject explicitly.
