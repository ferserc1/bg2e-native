# Lead-directed tracing capacity expansion

This correction supersedes the old 32/128/256 effect-profile and conservative
Metal 256-image/256-sampler/1024-total ceilings. It does not globalize material
ownership, change independently importable/exportable objects, or introduce a
ResourceManager, atlas, paged renderer or unbounded bindless architecture.

## Effective capacity

The latest shader/layout correction supersedes this report's initial ordinary
argument-buffer description for query shadows: fixed arrays use one Metal
argument buffer (including their unsized record-binding representation), not the
Tier-2 pointer-indexable record path required by draw tracing's larger tables.

**Direct-shadow follow-up:** SoftShadowPass also now follows this policy with
reserves `(2,1,9)`. Its old eight alpha-pair table and whole-scene neutral overflow
are removed. The query pass uses `draw.query.softShadows.v2`, Vulkan specialization
ID0 with bindings16/24 and Metal image/sampler records at buffers5/6. Paired albedo
resources remain counted as before; no shader/material texture-count optimization
is introduced. `alphaTextureCapacity()` and the existing reflected alphaTextureLimit
report the effective value. Per-slot neutral-alpha overflow retains scalar cutoff,
all instances and shadows from the rest of the scene; warnings are rate-limited.
Unsupported array hardware reports unavailable status and retains raster fallback.
The original shadow-table fixture now covers more than eight pairs and a forced
capacity9; no native execution is claimed. Texture-count improvements are deferred
by the lead until after API migration, outside this correction.

`gpu::MaxSampledResourceTableCapacity` is 16384. For each effect,
`ResourceArrayCapabilities::profileCapacity(I,S,R)` now returns the exact minimum:

```
min(16384, maxSampledImages-I, maxSamplers-S, (maxResourcesPerStage-R)/2)
```

An unavailable feature or exhausted reservation yields zero without unsigned
underflow. AO reserves `(4,2,13)`, GI `(5,3,15)`, reflections `(6,3,16)` as before.
Counts are not rounded down to old compiled profiles. The image and sampler tables
still share one capacity: separating their dimensions is future work. For example,
a sampler budget of 1024 yields GI/reflection capacity 1021, **not** 16384.

Vulkan keeps queried normal per-stage/pipeline-layout descriptor budgets and the
combined-resource bound; no update-after-bind or alternate larger feature limits
are claimed. Metal requires Tier 2, exposes its 1000000 indirect-texture bound and
queried `maxArgumentBufferSamplerCount` without the artificial 256 clamp. That
sampler property is an application-wide unique-sampler-description budget, used
conservatively here as a table-entry bound, not a new global sampler allocator.
Metal has no equivalent Vulkan aggregate descriptor count; independent type
budgets, direct binding validation and native argument-buffer byte limits apply.

## Exact native shader/layout agreement

- Vulkan adds UInt32 specialization constants to ShaderModuleDescription and
  ShaderLib::specialized. ShaderModule owns native map/data storage through pipeline
  creation. Compute/graphics/RT stages forward its VkSpecializationInfo. Shared
  tracing GLSL declares both arrays with specialization constant ID0; all relevant
  effect stages use the exact selected capacity, including non-power-of-two counts.
  These are fixed, fully populated descriptors after pipeline creation, not runtime
  descriptor arrays, partial binding or update-after-bind.
- Metal shared tracing uses pointer-indexed single-resource argument records:
  image buffer8 and sampler buffer9, record id0 in each. New primitive
  ResourceBinding::metalArgumentRecordArray requires an exclusive group and id0.
  ResourceSet encodes each record using the native encoder's encodedLength stride,
  checks buffer-size multiplication, retains native argument storage and declares
  each distinct indirect texture resident. Combined fixed argument groups used by
  the primitive GPU probe are preserved.
- AO/GI/reflection shader interfaces become `draw.rtao.v2`, `draw.rtgi.v2`, and
  `draw.rt_reflections.v2`. Reflection/settings persistence type IDs do **not**
  change. Custom native shader replacements must adopt the v2 binding contract.
- Existing `camera/world_{hash,blue}_256` files are deployment templates, no longer
  a promise of 256 runtime entries. Vulkan specializes them; Metal record indexing
  has no compiled array extent. Old 32/128 files remain historical/test templates.
  No `_1021` or `_16384` shader files or new CMake rules are required.

Packed 64/160/256-byte tracing records, six local material slots, object/resource
owners, GPU completion lifetime and the earlier `UINT32_MAX` neutral-texture
overflow fallback remain unchanged. No geometry/effect is removed on exhaustion.
Native descriptor/argument storage now scales with the selected bound; this is a
practical ceiling increase, not a resource streaming/performance refactor.

## Plan reconciliation and checks

Root index/operative decisions and M03 technical source/step notes identify this
as the later authoritative contract; original delivery/review/progress evidence
remains historical. M07-08 must document actual v2 capacities/bindings and neutral
fallback, M08 consumes them without reinstating old ceilings, M09 preserves the
deployed templates and specialization path, and M10 audits those maintained pages.

Static checks only: native/source API signatures and binding/record/specialization
lifetimes, capability arithmetic, shader include/deployment paths, licenses/style,
local links and whitespace. No configuration, C++/shader compilation, app, C++
probe, GPU, installation or package execution. The manual GPU probe adds CPU-only
16384/1021/132/aggregate/zero-capability cases; its native primitive subtest still
deliberately chooses a small precompiled profile. The draw RTAO snapshot regression
uses capacity37 (or a smaller native limit), checking overflow and recovery.

Lead validation: rebuild engine and both paired shader libraries; run the GPU
capacity helper and draw AO/GI/reflection probes, check effective capacities on
available Vulkan/Metal, and reopen primitives_scene.json with enabled GI/reflections.
Check non-power-of-two sizes, native diagnostics/residency, reload/resize/cleanup,
retained old snapshots, raster/no-RT fallback and overflow warnings. Native failure
must report effect status, never be disguised as observed support. Actual backend
coverage is not inferred from this source delivery.
