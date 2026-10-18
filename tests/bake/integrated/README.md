# Integrated bake manual fixture

`probe.hpp` supplies runIntegratedBakeProbe for an existing stopped draw Engine,
loaded scene/UV2 target, native shader root and explicit same-device environment.
Call it separately for RTAO and RTGI on available Metal/Vulkan configurations.
No target is added; this fixture has not been compiled or executed by the agent.
Required unsupported profiles throw; verify the explicit diagnostic rather than
accepting a neutral successful bake. GI also requires nonempty traceable geometry.

The fixture checks record-before-prepare, invalid slot/generation, duplicate
prepare/record, mismatched serial, globally reused serial across slots, completion
notification, exact sample indices/counts, raw sample ownership, cancellation with
pending work, reset generations, retained old samples, no premature composed result
and incomplete readPixels rejection followed by validated final output. Completion follows immediateSubmit's actual
GPU completion, not a guessed frame delay.

Additional application checks:

- Run bake commands alongside ordinary draw deferred presentation and picking;
  confirm independent descriptors/TLAS and unchanged raster/selection behavior.
- Submit normal acquired commands and use actual per-slot fences to call
  completeFrame. Continue presenting while the single bake sample is pending.
- Prepare then remove/disable/transform/reparent target or edit another source's
  material/light; verify prepare/record/completion rejection and no stale count.
  Stop GPU users before edits. Explicitly invalidate and reset to resume.
- Replace UV2 via regenerateDraw with context.invalidateSources as beforeChange;
  after adoption refresh selection/preview and reset the target. Re-extract BLAS
  identity and generation; old external sample owners remain independent.
- Record and reset/discard an unsubmitted command, then abandonFrame; verify no
  increment and a fresh snapshot/sample key on retry. Never abandon submitted work.
- Make a pass throw after partial recording; discard command, abandon and reset.
  Do not submit partial commands or retry the failed serial.
- Change live settings at a stopped boundary: frozenSettings is unchanged until
  reset. Reset must pick up the live object and start sampleIndex zero.
- Replace EnvironmentResources fields after construction without modifying image
  contents: the old recorded sample retains old image owners. Explicitly call
  setEnvironmentResources and reset to adopt a new immutable environment snapshot.
- Verify no-RT, missing shader, incompatible Device/capacity, excessive resources,
  GI empty geometry, absent GI environment and configured chart-unsafe denoise
  fail explicitly. AO may use neutral visibility for an empty traceable scene.
- Exercise source edits with unchanged pointers (texture uploads, CPU mesh edits)
  through explicit invalidation; automatic queue comparisons cannot observe them.
- Complete all requested samples: final validation publishes Complete/result/pixels;
  malformed, saturated or missing covered samples must fail with no output.
  Inspect raw signal/coverage independently from the padded multiplier.

Engine/Device outlive context and every externally retained sample. All commands
are retired or discarded before cleanup/destruction. See the
[API lifecycle](../../../doc/api/bake/IntegratedBakerContext.md).
