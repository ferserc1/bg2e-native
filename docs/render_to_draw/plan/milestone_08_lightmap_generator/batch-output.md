# M08-03 — Standalone batch and protected output

[Owning step](step_03_batch-output.md) · [Progress](../progress.md) ·
[Assembly](scene-assembly.md) · [Standalone context](standalone-context.md)

## Delivered API

`bake::StandaloneBakeBatch::run(engine, assembly, outputDirectory, options,
progressCallback)` consumes a private retained Assembly and a stopped initialized
offscreen draw Engine. It freezes a copy of Options, realizes deferred targets
without repeating already loaded custom hooks, and checks prepared drawable
identity/Engine ownership. CPU/uploaded target geometry must agree. UV2 generation
belongs to the assembler; the batch does not regenerate geometry.

Options carries LightmapSettings, byte image format, overwriteOutputs,
writeModelCopies, shaderRoot, optional environment, protectedInputs,
resolveDependencies and optional application job validity `current`. Progress is
zero-based targetIndex, targetCount, completedSamples and requestedSamples.
Progress runs before each target and after each completed sample, including the
last. False cancels without publication; callback exceptions propagate. Callbacks
are CPU-only and require stopped scene/resource producers. Settings mutations in
the caller do not change frozen jobs. Hidden custom mutable state must remain
stopped or be covered by `current`; arbitrary GPU writes are not monitored.

One StandaloneBakerContext prepares the whole scene. Each target gets its own
exclusive baker with independent frozen settings; it is cleaned/released before
the next. No generated map is applied to source materials between targets.
Readback follows completed composition/dilation. Result.bakedCount is assigned
only after successful publication; cancelled results contain no published paths
and report zero. Exceptions propagate as failure, not partial success.

`bake::LightmapOutputWriter` owns planning, protected-source inventory, staged map
encoding, optional detached model copies and commit. `setOverwriteExisting` must
precede `preflight`; all identities and output namespaces are planned before any
baker is created. `write` stages a preflighted identity once; `commit` requires
all targets, validates sources again and is one-shot. Writers borrow Engine/Device
lifetime. All new public types are included by `bake/all.hpp`.

## Protection and preservation

Preflight protects assembly/context/prefab/model files, all live standard material
image paths, environment images and referenced drawables, including disabled
nodes. `db::ObjMesh::sourcePaths` now reports the original OBJ and every successfully
read MTL, using the existing material parser. The writer supplements assembly's
inventory from those paths, even for directly loaded targets. Parsing and material
behavior are unchanged. Missing declared inputs fail closed.

Exact builtin component RTTI is recognized. Every other component, including a
subclass of a builtin, requires a complete BakeDependencyResolver declaration:
nullopt/missing resolver rejects, an explicit empty vector declares no external
files. Custom dependencies enumerate actual files, not directory placeholders.
Explicit protectedInputs may also reserve directories as output exclusion zones.
This does not discover arbitrary external files referenced only by custom code.

Paths used for IO retain lexical resolution semantics; weak canonical identities
(and Windows case normalization), ancestor checks and existing-file equivalence
prevent output/input aliasing, including symlinks and manifest hardlinks. Protected
file size/time/path checks run at source boundaries, with streamed content
fingerprints captured before work and rechecked at commit. Graph/component
membership/references, target geometry, current render queue/material/binding/image
revisions, lights and selected environment are compared. The baseline retains
native owners. Source fingerprinting is a stopped-work consistency check, not a
cryptographic integrity or hostile-process security facility.

The render API, native draw/gpu algorithms, shaders, revisions, pools and current
complete DB publication implementation are unchanged. The production DB
LightmapOutputWriter and old CLI remain available. Generic additions are
`db::publishFile` and a CPU TextureArchive write overload; existing source
signatures and GPU archive serialization are preserved.

## Output and publication contract

The authoritative entry is `outputDirectory/lightmaps.json`. It lists identity,
image, metadata and optional model paths **relative to that directory**. Every
referenced artifact belongs to a fresh `lightmaps-<unique-id>` generation.
Deterministic index-prefixed sanitized target names avoid case/path collisions.
Per-target metadata lives beside its image; each optional model has a separate
subdirectory containing model.bg2 and its complete unique native sidecars.

A private `.lightmaps-stage-<id>` directory contains all maps, metadata and model
copies until commit. `.lightmaps.lock` excludes cooperating writers to the same
physical directory. The complete stage is renamed into its unique generation;
then the pending manifest is atomically published LAST. Native publication uses
macOS exclusive rename, Linux renameat2 NOREPLACE and Windows MoveFileExW;
replacement is explicit. Unsupported native no-replace operations fail closed.
The portable fallback requires a hardlink. Readers must use the manifest instead
of scanning the output directory. Existing manifests reject unless overwrite was
explicitly enabled; symlink/nonregular manifests always reject.

Previous generations are retained even on overwrite. Cancel/failure before final
manifest publication preserves the previous complete product. Normal unwinding
removes owned staging and lock directories. A failure after generation rename
may leave an unreferenced complete generation. Process crashes can leave stale
locks/stages requiring manual inspection; automatic reclamation and previous
generation garbage collection are outside this step. This is atomic manifest
visibility for cooperating stopped producers, not simultaneous multi-path
replacement, hostile filesystem race protection or power-loss durability.

## Pixel and model semantics

RGB8 writes the explicitly selected PNG/JPEG/BMP/TGA format without another
exposure conversion. Lossless files are decoded and compared byte-for-byte;
JPEG is explicitly lossy and receives a dimension check. Metadata declares
linear RGB, top row v=0, UV2 (uvSet 1), indirectMultiplierRGB, bakedAO/bakedGI,
format/conversion/exposure, nearest-chart dilation and bake settings.

RGB32F writes a `.bg2tex` native RGBA32F archive, with alpha one and exact RGB bits.
The archive is read back and compared. No exposure, gamma conversion, clamp or
65504 cap is added. This output requires the draw native archive loader; it is not
a general interchange HDR image or a production texture-loader input.

Optional models clone independent CPU/GPU geometry/material/native texture
owners, apply the actual staged map as a **linear** AO-slot baked AO/GI RGB
multiplier with UV2/scale one/channel zero, and update clone materials. The
complete engine-aware `db::storeDrawableBg2` snapshot writes actual GPU texture
bytes into native sidecars; its bool-noexcept result is checked explicitly. The
source scene/material is never adopted or rewritten. Single-model exports carry
mesh/submesh placement/materials and chain joints, excluding node placement,
descendants and unrelated components; use scene export for a hierarchy. The
published model's sidecars no longer depend on staging paths.

## Verification and handoff

Static API/include/license/style, archive wire-layout, callback/ownership/source
and publication ordering checks, local Markdown link checks and git diff --check
are agent verification. No compilation, shader compilation, app, test probe or
GPU execution was performed. [Manual probes](../../../../tests/bake/batch/README.md)
cover CPU publication/archive, typed writer, complete model reload and the native
batch/cancellation/failure matrix for the lead. Native evidence remains pending.

M08-04 builds the CLI over these actual APIs and the manifest output layout.
M08-05 validates product behavior; M09 alone retires production compatibility.
No CMake modification is included in M08-03.
