# M04-06 — Build examples/draw/12_lightmaps with live GI disabled

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M04-01–M04-05.

## Scope and deliverable

- `examples/draw/10_deferred_renderer/src/main.cpp`
- `examples/draw/12_lightmaps/src/main.cpp`
- `examples/draw/12_lightmaps/CMakeLists.txt`
- `examples/CMakeLists.txt`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [examples/CMakeLists.txt](../../../../examples/CMakeLists.txt).

## Implementation work

1. Reuse the M01/M03 scene/delegate and disable live indirect RTAO/RTGI while preserving direct lights, environment/IBL, emission and baked-map composition.
2. Generate UV set 2 for suitable opaque target objects, safely rebuild them and prepare integrated bake jobs with progress/reset/cancel settings.
3. Apply completed maps to their original submesh/material definitions and live GPU materials with correct RGB multiplier/UV2 metadata; retain unbaked/transparent objects with explicit restrictions.
4. Add UV preview and configuration controls from actual bake objects. Register a runnable target/assets/shader libraries and report unsupported RT bake hardware without breaking scene display.

## Verification and exit conditions

- Baked indirect lighting is visible with live GI disabled; sample count/reset and cancellation have coherent progress/state.
- Target clone/geometry/material independence, UV coverage, viewport presentation and both output pixel formats have recorded results.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M05 derives from the non-bake deferred example; M06/M07 integrated bake UI uses this proven workflow, and M08 uses its validation but not its window scheduler.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-15 reusable product

Derive scene/application integration from examples/draw/10_deferred_renderer; see [working-example.md](../milestone_01_deferred_renderer/working-example.md). Its packed-channel fixture is material input/debug verification, not UV2 baking or a baked-lightmap implementation. Preserve shared delegate/settings/lifecycle integration when adding actual bake contexts.

## M03-10 reviewed implementation requirement

Start from the actual manipulation/deferred settings-window wiring, not assumed MainLoop auto-registration. When baked multipliers are applied explicitly disable live indirect AO/GI to avoid double lighting; decide reflection behavior separately. Verify chart isolation, full geometry reload and immutable snapshot ownership.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.


## M04-03 actual execution handoff

Use IntegratedBakerContext(engine,configuration), createBaker, prepareFrame/record and completeFrame only after the actual submitted frame fence completes. One target and one bake frame may be active per context; serialize targets or own separate exclusive contexts. Capture explicit EnvironmentResources snapshots and route replacements through setEnvironmentResources plus reset. Wire safe-edit beforeChange to invalidateSources and restart after adoption/selection refresh. Do not submit partially failed recordings; discard/abandon them. Finished texture output remains dependent on M04-04, not the raw latestSample API.

See [integrated execution](integrated-context.md).


## M04-04 actual output handoff

CompleteFrame now records exact completed progress and publishes final output after validating requested covered sample counts/numerics and reading the padded texture. Drive it from actual GPU completion, expect a final synchronous readback cost, and show explicit Failed/Cancelled/Invalidated states. Apply the retained RGBA32F RGB multiplier/UV2 metadata only after Complete; capture CPU output before material adoption invalidates source state. RGB32F remains HDR, RGB8 applies exposure only to GI. Optional denoise is rejected; do not enable display AA/denoise over exact accumulation.

See [output implementation](accumulation-output.md).


## M04-06 implementation handoff

The runnable `draw_lightmaps` target uses the shared example bundle helper and
existing engine shader deployment. No additional assets or private estimators.
The scene is static and generates independent floor/cube/clone UV2 replacements
from retained original drawables. One synchronous `Device::immediateSubmit`
per prepareFrame completes before `completeFrame`; no general render-loop
callback is required. Unsupported bake effects/profiles leave scene display
available and report the reason. UI edits actual idle LightmapSettings; partial
results are previewable but never adopted. Cancel/restart and all application
mutations run before window acquisition. Results and CPU pixels are captured for
all targets before context teardown and material adoption.

MaterialAttributes now carries explicit IndirectTextureMode metadata for the AO
texture slot, serialized as ao/bakedAO/bakedGI. The G-buffer previously wrote only
neutral one into its reserved baked output; it now samples marked RGB UV2 maps,
bypasses scalar AO and retains HDR using RGBA16F. Deferred shader variants preserve
direct/emission and specular IBL for GI, and bypass live AO/GI on marked pixels.
The example globally disables those live passes and independently disables live
reflections. Half-float display overflow is rejected, while bake/CPU float output
remains RGBA32F/RGB32F. Stored definitions receive metadata, but GPU-only image
sources still require real materialization in future export steps.

[Maintained material API](../../../../doc/api/draw/BakedIndirectMaps.md) and
[example guide/manual checks](../../../../examples/draw/12_lightmaps/README.md)
record lifecycle, limitations and the lead validation matrix. The lead must
compile/run the new target and existing draw examples on available Vulkan/Metal;
no agent build, shader compilation or application execution is claimed.
