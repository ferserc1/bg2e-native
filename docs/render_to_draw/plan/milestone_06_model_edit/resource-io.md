# M06-02 resource IO implementation and review handoff

Delivery date: 2026-10-19. Starting and finishing HEAD:
`bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d`; all changes remain uncommitted.
Inherited/staged changes were preserved, including M06-01 and tutorial 13. The
lead's continuation accepts M06-01; backend/scenario coverage is unspecified.
No CMake file was modified in this step. No configure, compile, shader tool,
application, C++ probe, packaging or GPU execution was performed.

## Actual contracts

The maintained [complete IO reference](../../../../doc/api/db/DrawResourceIO.md)
is the API/ownership/format contract. New source is
`lib/src/bg2e/db/draw_resource_io.cpp` plus `texture_archive.cpp`/its public header.
Explicit `saveScene(root, drawEngine, path, progress)` and
`storeDrawableBg2(path, drawEngine, node)` snapshot actual realized resources.
Raw/schema/reference IO remains engine-free and deliberately incomplete.
Production overloads remain distinct and operational source paths are retained.

Shared cgltf geometry/material parsing now has a draw realization overload;
static import limitations are explicit in the API reference. The tinyobj geometry
reader also serves MTL-aware draw import, splitting shape/material runs and
resolving each material's texture directory. BG2 draw Node import retains joints;
BG2 CPU data now carries separate submesh names, while duplicate named material
definitions are matched by occurrence rather than losing independent edits.
Low-level BG2 output has checked writes/close, corrected directory joining and
RAII buffers. Invalid geometry indexes/truncated polylists fail before adoption.

Native `.bg2tex` version 1 stores exact color mip/layer bytes and format. Draw
Texture realizes it without conversion/regeneration. Material JSON adds sampler
and color fields, retaining UV/channel/inversion/indirect metadata; unlit now
round-trips with compatibility for the older alternate key. Shared JSON string
encoding/decoding handles escaped paths/names/control characters/Unicode. Native
archives require little-endian hosts and draw loaders; production PNG/HDR loaders
are not silently redirected to this format.

Publication uses unique staging/generation paths beside the destination. It
publishes sidecars without replacement and atomically replaces the main file
last; ordinary failure rolls back only this generation. Previous generations
remain intentionally. Node and custom component reference hooks preserve graph
fields, joints and display names; unsupported hooks fail. Temporary Drawable
references restore by noexcept exchange. Custom external resource types need an
explicit component-owned export contract. One-node BG2 rejects unrepresentable
graph/Node state; complete scenes retain it. See the API reference for filesystem,
image usage, import subset and power-loss boundaries.

Document/StageScene/AppDelegate implement candidate load then stopped adoption,
source/save paths, dirty/generation metadata, Open/Save/Save as/drop flows and
Save/Discard/Cancel on replacement/close. Save marks clean only after publication.
Ordinary IO runs before acquisition in prepareFrame; close-save runs between
frames with device drain. Renderer queues/catalogs reattach before retired model
resources are cleaned. The floor/camera/light remain editor fixtures; exports
start from the document root. Future edits must mark Document modified and guard
queued targets by document generation, beyond renderer generation alone.

## Static evidence and lead checks

Static checks cover 27 owned C++ files for full GPL headers, lexical delimiter
balance and opening-brace style; manual declaration/definition, ownership,
publication/rollback, format/metadata and namespace/dependency review; maintained
Markdown links and balanced fences; working-tree and index `git diff --check`.
This does not establish compilation or backend correctness.

The [manual probe and scenario matrix](../../../../tests/draw/resource_io/README.md)
cover GPU-only HDR/mips/bakedGI/UV2/sampler round-trip, JSON escapes, failed custom
serialization rollback, BG2 names/joints/duplicate materials, OBJ/MTL directories,
static glTF/embedded images, custom schema hooks, archive rejection, failed IO,
unsaved dialogs and production coexistence. The lead should build/reconfigure to
discover new source files and execute applicable checks on available backends.

M06-02 is delivered awaiting project lead validation. Next is M06-03; no tools,
material widgets, framing or bake integration are claimed here. M06-07 must review
these IO boundaries together with the completed application, and M06-08 must audit
all maintained pages cumulatively.
