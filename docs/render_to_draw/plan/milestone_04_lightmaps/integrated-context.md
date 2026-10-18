# M04-03 integrated context implementation

Baseline HEAD: 95190d6d042bb85d6e9d72153b155192dac64377, preserving uncommitted
M04-01/M04-02 deliveries. M04-02 accepted by project lead continuation; backend
and scenario coverage unspecified. New implementation lives entirely in bake.

Delivered IntegratedBakerContext, IntegratedLightmapBaker and LightmapBakeExecutor
with shared public draw UV/AO/GI primitives, exclusive selected-pass instances,
bake-owned matching-capacity TracingSnapshots/TLAS/resource bindings, full-resolution
frozen settings and explicit world-surface native profile selection. No new
shader family, renderer, production-render code or GPU primitive is required.

Actual lifecycle and limitations are maintained in the
[API reference](../../../../doc/api/bake/IntegratedBakerContext.md).
One live target and one in-flight bake frame per context serialize exact raw sample
sequencing while presentation uses its own resources. CompleteFrame must follow
actual GPU completion; record never advances completedSamples. Abandon only follows
reset/discard of unsubmitted commands. Duplicate/stale command/slot/serial/source
jobs are rejected; target/source edits are rechecked, with explicit invalidation
required for in-place resource contents. Cancel retains pending owners. Reset at a
stopped boundary freezes settings anew and advances target generation.

Environment structs are copied retaining image/sampler owners. Explicit environment
replacement invalidates/reconfigures the context without a second renderer. Gizmos
must be outside the supplied bake root or nontraceable; no renderer/UI overlay
queue is read. The selected public pass builds a fresh per-recording snapshot and
retains partially recorded owners. Failure recovery discards unsubmitted commands,
retires/abandons the open frame, and resets before further recording.

M04-04 consumes completed BakeSample.signal and surface chart/coverage/position/
normal owners, preserving raw AO visibility or GI incoming radiance/pi plus alpha
validity. No accumulation, irradiance normalization, dilation, preview or finished
LightmapResult/readPixels is claimed. Progress remains Running with composition
pending after requested raw samples. Chart-unsafe denoise is explicitly rejected.
An unavailable required effect/format/profile fails; empty traceable GI fails.

[Manual fixture](../../../../tests/bake/integrated/README.md) and additional checks
cover lifecycle/generation/replacement, presentation/selection isolation, missing
profiles, no-RT, cancellation, abort and environment/resource ownership. Agent
verification is static only; no compiler, shader tool or C++/GPU fixture executed.
Native numerical/performance evidence remains project-lead validation work.
