# M02-07 — Build examples/draw/11_manipulation and modifier validation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M02-01–M02-06.

## Scope and deliverable

- `examples/draw/10_deferred_renderer/src/main.cpp`
- `examples/draw/11_manipulation/src/main.cpp`
- `examples/draw/11_manipulation/CMakeLists.txt`
- `examples/CMakeLists.txt`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [examples/CMakeLists.txt](../../../../examples/CMakeLists.txt).

## Implementation work

1. Derive the M01 scene and non-template delegate, adding selectable nodes/submeshes and optional overlay integration.
2. Provide explicit MaterialModifier presets/JSON patches applied to selected submeshes, including scalar, texture, alpha/UV and group/regex examples. Keep the example focused on manipulation and API validation.
3. Add controls for clone then modify independently, selection input capture and optional viewpoint measurement. Reuse settings/upscaling/shader libraries rather than duplicate engine code.
4. Register a unique executable target and packaged resources; preserve the original deferred example as a non-editor baseline.
5. Keep example input/window code behind engine wrappers: use `app::Keyboard::controlPressed()` and logical `app::MainLoop::windowSize()` rather than including or calling SDL. Construct the base delegate with an owned `RendererDeferred` plus the overlay configuration. UI cleanup callbacks require valid weak shared ownership.

## Verification and exit conditions

- Select and modify a material through the specified utility, including texture refresh and preservation of unrelated live state on both available backends.
- DPI/scale picking, additive/deselect/delete, gizmo transform and cloned GPU independence have recorded results; no-RT rendering stays functional.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M03 verifies both existing examples after effects integration. M06/M07 use the validated MaterialModifier/widget/selection APIs.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-15 scene product to derive from

Reuse the actual examples/draw/10_deferred_renderer product and [working-example.md](../milestone_01_deferred_renderer/working-example.md), not an assumed private renderer loop. Retain the shared facade/window, queued replacement/material edit generation checks, native-free app resampler and CountingGBuffer integration. The scene supplies OBJ/clones/glass/cutout/packed-UV fixtures; selection/gizmos remain this milestone's work.

## M02-01 actual handoff

Use the M02-01 typed selection/gizmo contracts in dual-manipulation.md. Call refreshSelection after scene/resource edits, release all instance-owned draw gizmo resources before Engine cleanup, and keep optional tools out of ordinary lighting/query geometry. Existing MainLoop safe callbacks use weak component/node lifetimes.

## M02-02 actual picking handoff

See [picking.md](picking.md) for synchronous submission-local IDs, typed selection,
unjittered/top-left viewport coordinates, setPickPixelScale and stopped-producer
requirements. Reuse RGBA8 geometric picking; no depth readback was added.
Route input at a safe boundary and refresh DPI scale on window changes.

## M02-03 implemented overlay integration

Use [the actual overlay contract](overlays.md): register a shared
GizmoAndSelectionRenderer in RendererConfiguration.overlays before initialization.
RendererDeferred drives prepare/update/post-final record and cleanup. MeshOverlayPass
provides public shader/graphics replacement and explicit geometric depth/line/blend
policies at output resolution without jitter. Prepare mesh topology before acquisition,
request a scene frame after selection edits, and retain Engine through tool cleanup.
Do not add private renderer target access or draw tools into ordinary scene extraction.

## M02-04 actual material patch handoff

Use [draw MaterialModifier contracts](material-modifier.md): materialDefinition()
is the stored CPU target, while material() aliases live state after loading.
Drawable patches affect both independently; direct MaterialBase patches are live
only. Queue at a stopped boundary, validate weak targets, request redraw and reset
histories even on partial failure. Existing updateTextures flushes pending slots;
callbacks/errors are nontransactional. alphaCutoff is supported by the shared decoder.

## M02-05 actual widget integration

Use [the widget decision table](widgets.md). DrawMaterialEditor binds Engine plus
a shared DefaultRenderLoopDelegate and a typed draw selection (or explicit weak
material), then queues partial edits. DrawTextureWidgets/TexturePreview use the
draw UserInterface session, retain actual GPU snapshots and retire registrations
after completion. No image upload or material setter belongs in UI draw. Use the
safe onChanged(Renderer&) callback for future history invalidation. ComponentInspector
and raw scene/property mutations require the later queued app adaptation; they
are not certified as draw-safe by M02. UV/bake previews remain with M04.

## M02-06 actual viewpoint integration

Use [the draw analyzer contract](viewpoint-analysis.md) for the optional framing action. Run analysis outside frame/UI preparation at a stopped scene/GPU boundary, map results through `analyzedSubmeshes()`, and apply the selected `utils::CameraCandidate` with the draw camera helper. Keep viewpoint raster-only; no RT availability is required.
