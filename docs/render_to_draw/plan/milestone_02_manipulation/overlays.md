# M02-03 delivered overlay contracts

Date: 2026-10-16. Source baseline `7b217bcb6d9a3473ea522dd258d8d87b87150ddb`,
with the uncommitted M02-02 delivery preserved. M02-02 is accepted by project lead
continuation; no additional backend/scenario evidence was supplied this turn.

## Public attachment and borrowed frame seam

`draw::RendererConfiguration::overlays` retains an ordered vector of shared
`draw::pipeline::OverlayPass` instances. Empty is the default. Null/duplicate
registrations fail before initialization; an instance belongs to one renderer
lifecycle. RendererDeferred initializes with output color/depth formats, output
extent and acquired-slot count; attaches scenes, prepares before acquisition,
updates after scene animation/extraction, records after reconstructed HDR and
final color processing, resizes and cleans up in reverse overlay order. Partial
initialization is included in cleanup. Surface format changes use existing full
renderer lifecycle reinitialization.

`OverlayContext` borrows the current FrameContext (including output image), scene
root, canonical queue and original unjittered view/projection. It is valid only
for the current callback. The slot is acquired and its prior submission complete;
resources recorded now must survive GPU completion. This callback seam is the
minimal surface/camera access: no private renderer targets or lasting getters
are exposed. Extensions must close command scopes, never wait/submit/read back
or mutate the scene during update/record, and leave final color usable for sampling.
No production Vulkan implementation is added to manipulation or draw.

## Replaceable graphics component

`draw::pipeline::MeshOverlayPass` is a public virtual GPU graphics component with
`MeshOverlaySettings`, initialize/resize/record/cleanup and explicit
`OverlayDrawList` ownership. It has no manipulation dependencies. The high-level
`manipulation::GizmoAndSelectionRenderer` now implements OverlayPass and accepts
an injected shared MeshOverlayPass. Applications may replace the entire extension,
the graphics component or either paired shader library. Existing production
init/native draw/accessors stay available. Draw `init(engine)` retains mode-only
CPU readiness; attached tools are initialized by RendererDeferred directly.

Settings select highlight/type wire or filled topology, blending and scene-depth
testing (with configurable small normalized-depth bias for tested highlights);
shaders are explicit paths with a documented common ABI. Defaults retain
production wire highlights/type icons, alpha blending and no scene-depth test.
Transform handles use filled unlit material colors, opaque depth-tested writing
by default, and last-draw ordering. `setLineIntensity` remains live highlight
opacity (finite, clamped0–1 for draw). Portable lines are one output pixel; no
optional native polygon/wide-line feature is required or added.

MeshOverlayPass owns output-resolution scene/transform depth per acquired slot.
Optional scene-depth policy rerasterizes visible queue triangles with the original
camera and masked color writes, not a scaled/jittered renderer depth attachment.
It is geometric, without material alpha sampling, including transparent surfaces.
Transform handles use separate fresh depth in a new rendering scope, retaining
final color with explicit ShaderReadOnly/ColorAttachment dependencies. This is
compatible with Metal's requirement to clear before encoder materialization and
keeps transform geometry out of scene/FSR depth and histories. Overlay colors are
written directly in the final attachment's color domain; no tone mapping is
applied to them. Output returns to ShaderReadOnly for presentation or another pass.

The 80-byte vertex push block is mat4 mvp + vec4 color, Metal buffer1, position
attribute0 using the engine PNUUT layout. Highlight and gizmo have separate
paired GLSL/Metal libraries under draw/selection_highlight and draw/gizmo.
Existing recursive source/shader discovery includes them; no CMake edit is needed.

## Resource and selection lifecycle

GizmoAndSelectionRenderer prepareFrame synchronizes optional GizmoComponent assets
and caches explicit undirected triangle edges for each loaded mesh. Edge indices
are deduplicated within each submesh, preserving slot boundaries and all triangle
edges. Immutable GPU mesh identity keys an instance-owned cache; a new mesh is
prepared before acquisition. No buffers/pipelines/images are created while
recording. CPU-mirror mutation of a realized GPU mesh is not supported; replace
or rebuild topology at a stopped boundary. Expired source entries are pruned.
The cache is not a generalized ResourceManager or shared global cache.

After scene update, the overlay snapshots canonical visible selected submeshes,
live material colors and synchronized tool geometry with current world transforms.
CPU Selectable identities are refreshed before selected bits are consumed. Type
icons honor global type visibility/opacity and transform handles honor per-handle
visibility and live axis/active colors. Disabled nodes/ancestors are excluded.
Tool geometry stays outside normal lighting and RT extraction.

Each acquired slot retains its immutable draw list and GPU owners until slot
completion. The graphics component also retains its submitted list by slot, so
standalone callers may discard their CPU list after recording. Scene replacement,
resize, cleanup and renderer replacement happen at stopped/drained boundaries.
Borrowed Engine/device must outlive all tools/resources. Empty configuration
requires no manipulation state; offscreen callers omit overlays unchanged.
Selection/visibility edits request a scene frame in non-continuous mode; a fully
paused scene retains its image until resumed. The M02-02 picking/input path is
reused and no picking algorithm or new executable is added by this step.

## Verification and handoff

Static review covered include/caller signatures, dependency direction, virtual
lifecycle/partial failure, acquired-slot retention, shader ABI/entry names,
color/depth/load-store dependencies, unjittered output sizing, edge topology and
GPL/style/whitespace. No compilation, shader compilation, runtime/image check or
CMake change was performed. The instructed skills.md remains absent; repository
architecture.md and plan architecture/review contracts provide source context.

[Lead integration and backend/production checks](../../../../tests/manipulation/overlays/README.md)
cover intermediate injection, selection, depth/blend/line controls, FSR/scale,
gizmos/input, replacement/cleanup and no-overlay/no-RT cases. M02-07 supplies the
final manipulation example; it is not claimed to exist yet. M02-03 awaits lead
validation; M02-04 MaterialModifier follows acceptance.
