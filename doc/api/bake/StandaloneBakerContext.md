# StandaloneBakerContext

**Header:** `<bg2e/bake/StandaloneBakerContext.hpp>`; also in `<bg2e/bake/all.hpp>`.
**Namespace:** `bg2e::bake`. This is the experimental draw/bake path; production
render baking remains available separately.

The context borrows an initialized **windowless draw Engine** and owns a private
serial UV job lifecycle. It neither creates another Engine nor obtains effects
from a camera renderer. Engine and Device must outlive context, bakers, targets
and externally retained GPU results. Stop all scene/resource producers throughout
preparation, samples, reset and cleanup.

## Operations and states

| Member | Contract |
|---|---|
| `StandaloneBakerContext(draw::Engine&)` | Borrow the original initialized offscreen Engine/Device identity. |
| `initialize(std::filesystem::path shaderRoot = {}, BakePassConfiguration passes = {})` | Created → Initialized; one private slot regardless of surface slot count. Empty root selects deployed shaders. |
| `prepareScene(std::shared_ptr<scene::Node>, std::shared_ptr<const draw::environment::EnvironmentResources> = nullptr)` | Retain a loaded root and prepare a new source generation. Null environment generates the selected authored/default sky through public environment tools. |
| `createBaker(LightmapBakerConfiguration)` | Return the exclusive [StandaloneLightmapBaker](StandaloneLightmapBaker.md). Only one live target handle is allowed. |
| `invalidateSources()` | Retire current scene/jobs and require another prepareScene/new baker. |
| `cleanup()` | Idempotent stopped cleanup; does not clean the borrowed Engine. |
| `state()`, `configuration()`, `sourceGeneration()` | Inspect lifecycle, frozen context configuration and current source generation. |

States are Created, Initialized, SceneReady and Cleaned. Every preparation attempt
invalidates old jobs first; failure leaves Initialized, not a successful old
SceneReady generation. Release the previous baker before creating another, even
after preparing a new scene. Stale jobs cannot reset into a new scene generation.

The caller loads resources, updates the scene and prepares UV2 **before** freezing
sources. Retain an Assembly's Scene alongside its root to preserve scene-owned
selection/caches. Environment configuration is copied while retaining shared
image/sampler owners. Explicit resources must belong to the same Device.

## Public pass reuse and completion

BakePassConfiguration can supply exclusive public UV/AO/GI, accumulator and
composition objects; null entries select bake defaults. Their initialization,
format/capability/interface and chart/signal contracts must be compatible. A
DenoiseFilter without chart isolation is rejected. Passes must not share mutable
slots with display rendering or another job. The batch/CLI expose compatible
LightmapSettings rather than arbitrary camera-renderer profiles.

Creating/resetting a target validates required tracing/kernel/capacity support
and primes a real completed tracing snapshot/TLAS without counting a sample.
Each update creates a logical snapshot wrapper; existing immutable tables and
exactly matching completed acceleration allocations may be reused. Independent
output owners remain retained through completion; a new wrapper does not require
a fresh native allocation. Current v3 tracing, v5 shadows and device-clamped
capacity are not replaced by an older fixed 256-entry policy.

Source validity checks current queue, transforms, material/binding/classification
revisions, native image generations, BLAS build generations and environment
identity. The original Device timing identity also detects replacement at a reused
address. Unobservable in-place resource writes/custom state still require stopped
producers and explicit invalidation; signatures do not make arbitrary mutation
safe. Required unsupported baking fails rather than returning a no-RT neutral map.

Work submissions and completed readback are synchronous. These are workload
completion boundaries, separate from asynchronous GPU profiling; do not add timing
waits to poll measurements. Per-target cleanup releases its services; no
cross-target cache-sharing speedup is promised.

For a complete job see the [standalone tutorial](../../tutorials/draw/standalone_lightmaps.md)
and [batch API](StandaloneBakeBatch.md). Sources:
[header](../../../lib/include/bg2e/bake/StandaloneBakerContext.hpp),
[implementation](../../../lib/src/bg2e/bake/StandaloneBakerContext.cpp).
