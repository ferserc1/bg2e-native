# M08-01 — Standalone UV bake execution delivered

[Step](step_01_standalone-context.md) · [Progress](../progress.md) · [Source preservation](../code-reconciliation-2026-10-21.md)

Baseline HEAD: `78943d5820e69363c942d71136f479a0df87128e`, with inherited uncommitted M07 documentation and plan-audit changes preserved. The lead requests continuation after the audit; M07 documentation is already accepted. No independent optimization/profiling runtime or performance acceptance is inferred. This delivery implements M08-01 only.

## Actual public contract

Headers: [StandaloneBakerContext.hpp](../../../../lib/include/bg2e/bake/StandaloneBakerContext.hpp), [StandaloneLightmapBaker.hpp](../../../../lib/include/bg2e/bake/StandaloneLightmapBaker.hpp); included by bake/all.hpp. Implementation: [StandaloneBakerContext.cpp](../../../../lib/src/bg2e/bake/StandaloneBakerContext.cpp).

`StandaloneBakerContext(draw::Engine&)` borrows an already initialized **windowless** draw Engine. The existing Engine offscreen overload owns Instance/OffscreenSurface/Device; this bake scheduler does not duplicate those owners or instantiate a camera renderer, application driver or UI. The Engine and Device must outlive the context, every baker/target and externally retained GPU result. A device identity check rejects reinitializing/replacing the borrowed Device even if an allocator reuses its address.

- Created → `initialize(shaderRoot, passes)` → Initialized. One private serial slot is configured, independently of surface/presentation slot count. The context configuration is intentionally incomplete until a scene is prepared.
- `prepareScene(loadedRoot, optionalEnvironment)` invalidates old scene jobs before attempting publication. Null environment invokes public EnvironmentGenerator against the authored queue selection, including default/null selection and procedural colors. Explicit snapshots are validated and copied by value, retaining native image/sampler owners. Repeated automatic preparation resets the file-source cache. Failure leaves Initialized and old jobs invalid; it does not bless old sources.
- SceneReady → `createBaker(LightmapBakerConfiguration)` validates a target and frozen settings, initializes public AO/GI/UV/accumulation/composition primitives and preflights a matching-capacity snapshot/TLAS to actual synchronous completion. This preflight counts no sample. Required capability/kernel/format and unsupported empty-GI-scene failures throw. Only one live baker is allowed: release it before the next target, including after a new scene generation.
- `StandaloneLightmapBaker::update()` prepares a fresh snapshot wrapper, records raw UV AO/GI, exact accumulation and (on the last update) composition/dilation, then submits synchronously through Device::immediateSubmit. The native implementations wait for that submission. Exact RecordingCompletion is also checked before publication. Only completed validated updates increment completedSamples. Final validation and typed readback precede Complete/result publication.
- `cancel()` hides output and rejects further updates. Cancellation during an injected pass cannot publish the completed pending sample. `resetAccumulation()` starts a new target generation and refreezes live settings **only in the same current scene generation**. Old scene bakers cannot revive through reset. `record(BakeFrame)` accepts only the exact internal command/slot/serial once; arbitrary external recording is rejected.
- `invalidateSources()` retires the source generation and requires prepareScene plus a new baker. `cleanup()` is idempotent, drains resources and cleans the active target/generator while the Engine remains alive; it never cleans the borrowed Engine. Context transitions to Cleaned and cannot be reinitialized.

Minimal source-level use with a loaded UV2 target, after caller-owned scene updates:

```cpp
bg2e::bake::StandaloneBakerContext context(headlessEngine);
context.initialize(shaderRoot);
context.prepareScene(root, environment);
bg2e::bake::LightmapBakerConfiguration configuration;
configuration.target = target;
configuration.settings->setMode(bg2e::bake::LightmapMode::RTAO);
auto baker = context.createBaker(configuration);
while (baker->progress().state != bg2e::bake::BakeState::Complete)
{
    baker->update();
}
auto pixels = baker->readPixels();
baker.reset();
context.cleanup();
```

This recipe is checked against declarations, not compiled/executed. The caller handles exceptions and output; no standalone export/CLI target is introduced here.

## Source validity, ownership and preservation

The prepared queue retains mesh/material/resource owners and copied lights/environment intent. Before and after samples, extraction compares node/submesh correspondence, transforms, material values/classification/binding revisions, texture and native image generations; explicit BLAS build and environment image generations are captured separately. Target membership, enabled ancestry and loaded draw mesh identity are checked. Changes invalidate the scene or reject the target before publishing output. Unobservable in-place GPU uploads/custom changes still require explicit invalidateSources before mutation; callers own scene update and producer coordination.

Public LightmapBakeExecutor, UvTemporalAccumulator and LightmapCompositionPass implement tracing/signal/coverage/chart/count/HDR/dilation behavior; there is no copied effect algorithm or production rendering call. Immutable table and completed acceleration reuse remain active across samples. No forced full TLAS rebuild, alpha/default/material ownership change, table-capacity reduction or native ABI change is made. Per-target cleanup releases that target's services; no cross-target cache-sharing speedup is claimed.

The shared implementation-only StandaloneBakeState is necessary to keep context validity alive for late baker handles. The target Impl owns exclusive execution resources and staged outputs; it is not a private replacement for public passes. Both follow the existing integrated context's handle/lifetime pattern while owning a separate serial scheduler. No integrated scheduler is used.

Partial-record exceptions abandon the immediate command and never increment counts. Failed submission/wait/validation clears public output and drains before dropping pending owners. If draining itself fails, the context marks unsafe and rejects further preparation/update; explicit cleanup must successfully retire work before releasing resources/Engine. Normal native-device lifetime preconditions remain necessary. All methods are serial, on a stopped producer thread; concurrent cancellation/graph mutation is not promised.

RGB32F output preserves the public composition semantics without imposing Composer's interactive half-range adoption limit. Existing raw GI saturation validation still applies. No chart-unsafe denoise is introduced; the executor retains its explicit rejection of unsupported filtering. Environment-only empty-GI behavior remains unsupported, matching the executor. CPU profiling scopes/counters identify prepare/reset/update/completed samples; no timing-only GPU wait or headless Performance window is added.

## Verification and handoff

Static checks: include/declaration/caller and native immediateSubmit completion contracts; generation/reset/cancel/error transitions; queue/native resource keys; output ownership and final metadata; dependency confinement; GPL/control-flow/function spacing/method groups; Markdown paths and diff whitespace. `skills.md` remains absent at repository root; no CMake/source configuration workaround is introduced.

[Manual fixture](../../../../tests/bake/standalone/README.md) supplies runStandaloneBakeProbe for two loaded targets, frozen/reset settings, external/duplicate lifecycle misuse, partial-record failure/recovery, cancellation during accumulation, stale scene reset/update, retained outputs and cleanup. Run supported AO/GI cases on available Metal/Vulkan and separately check missing RT/arrays/formats/shaders, authored/default/explicit environments, source/native changes and native submit/wait failures. No configure/build, shader compiler, C++ fixture, app/GPU or package execution was performed by the agent. Runtime and compilation acceptance belongs to the project lead.

M08-01 is delivered awaiting project lead validation. [M08-02](step_02_scene-assembly.md) is the next step; it must prepare loaded immutable model/prefab sources and UV2 before prepareScene, release stale/previous bakers before successor jobs, and preserve camera-less roots. Batch writer/CLI remain M08-03/M08-04, and cumulative maintained documentation closes at M08-07.
