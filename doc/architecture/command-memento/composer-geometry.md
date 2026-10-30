# Composer geometry

## Prepared geometry versions

`GeometryEdit` supports axis conversion, centering, floor placement, unit
conversion and prepared geometry replacement with ray tracing enabled or disabled.
Commands prepare immutable before/after CPU snapshots and the new native version
before history admission. Every document Drawable sharing the CPU Mesh receives
its own prepared raster buffers and, when supported and enabled, one BLAS per
submesh. The BLAS borrow exactly those prepared vertex/index buffers.

CPU geometry is deduplicated by Mesh identity and published once per shared mesh.
Materials, Drawable identities and submesh membership remain unchanged. Vertex
and index counts may change; the number of submeshes must remain fixed.

## Publication and replay

Known geometry producers stop before preparation and publication. Before each
exchange Composer drains outstanding GPU resource readers. All fallible copies,
uploads and BLAS builds finish before live CPU data or native bindings change.
The batch then swaps CPU payloads and exchanges raster buffers and dependent
BLAS together. Preparation failure leaves the live version unchanged. An
unexpected partial exchange is compensated; failed compensation faults history.

The command keeps the displaced complete native version. Undo and redo exchange
that version with the live one, using exact captured CPU payloads rather than
reapplying inverse arithmetic or rebuilding BLAS. Axis conversion maps
`(x,y,z)` to `(x,-z,y)`; normal and tangent magnitudes, zero vectors, winding and
UV sets are preserved by the supported transforms. Redo does not recalculate
bounds from the current selection.

Replay checks document membership, Mesh/Drawable identity, live CPU/raster
agreement, BLAS buffer/range correspondence, and the expected CPU snapshot.
Changes to device, tracing eligibility, or unsupported external writes reject.
New document aliases require a new command rather than silently retaining stale
native geometry. SDK owners sharing mutable CPU geometry outside the Composer
document remain responsible for their own synchronization and rebuilds.

## Memory and retirement

Admission reserves both CPU snapshots, conservative CPU/raster/upload overhead,
and the actual retained BLAS and scratch allocation sizes of the two versions.
Preparing candidates requires temporary allocation before admission; an exhausted
history budget rejects the command without publishing the candidate.

Native resource identities are registered with the deferred release ledger even
when their bytes are already reserved locally. Eviction and redo-tail discard
therefore request a GPU drain before destroying command-owned native versions.
Each version retains raster buffers alongside its dependent BLAS. Renderer
acceleration keys distinguish native mesh and BLAS identities/generations, so a
geometry exchange selects a compatible TLAS generation on the next extraction.

This implementation favors retained complete versions and conservative device
waits over rebuilding on replay, in-place BLAS updates, compaction, or fine-grained
fence retirement. It uses more history memory to keep replay deterministic.
