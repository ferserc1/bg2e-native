# Phase 08 Temporal invalidation

## Outcome

Unchanged scene signature components are reused instead of reconstructing and comparing the full serialized signature every frame. Proven effect-specific resets avoid unnecessary history loss while retaining conservative correctness for motion and edits.

## Implementation

1. Replace repeated serialization of unchanged materials, textures, geometry, and settings with revision-keyed signature components. Retain deterministic correspondence checks and identities independent of dense queue indices.
2. Preserve the current effective invalidation policy initially, including moving-object resets. Demonstrate signature construction savings before changing which histories survive.
3. Partition invalidation by actual effect dependencies where independence is demonstrable. Keep global generation resets for changes that affect the whole scene. Material or light changes can affect distant GI, reflections, and shadows; do not assume a local edit permits only a local reset.
4. Audit object motion correspondence, previous surface data, disocclusion rejection, transparent layer identity, and camera resets before retaining history across any additional motion. The current renderer does not justify removing all transform invalidation merely because it has reprojection helpers.
5. Apply narrower resets only for verified cases and keep conservative resets for unsupported dynamic correspondence. More complete object-motion reprojection is not a prerequisite for this phase's signature reuse optimization.
6. Reuse source revision infrastructure without exposing cache addresses or renderer material indices as stable object identities.

## Verification and exit

Compare invalidation decisions with the prior full signature path. Test static scenes, camera changes, object motion, add/remove/reorder, texture replacement, material edits, lighting/environment changes, cutoff crossings, and transparent layers. Check accumulation convergence and ghosting, not only single-frame output.

Measure signature bytes rebuilt, component cache hits, CPU time, and reset counts by cause. Exit with lower unchanged-signature work and no weakened history correctness. Any narrower-reset case must have its own visual evidence; otherwise retain its conservative reset and record the reason.

## Implementation record

Status: source implementation complete; native compilation, reference-probe
execution, timings, convergence captures and ghosting comparisons remain pending.
No narrower reset or visual acceptance is claimed.

- RendererDeferred retains immutable component signatures for frame configuration,
  ordered geometry/correspondence, exact object transforms, effective material and
  texture identities, lights, environment, noise and effect settings. Material keys
  reuse phase 02 captured identity and values/bindings/classification revisions;
  native identities/revisions guard component lookup on transition frames.
- Warm scene checks reuse the complete state without per-object map allocation,
  payload serialization or full-vector comparison. Global-only changes retain all
  instance components. Dirty frames serialize changed components only. Source
  identities/owners remain separate from derived table addresses; lookup caches
  retain only the latest collection plus the accepted history state.
- Mutable settings and virtual denoise getters are checked through explicit typed
  snapshots. They do not have reliable setter-only authored revisions. Fieldwise
  bit comparisons preserve the reference's exact effective values without comparing
  settings padding. Light/environment public fields remain directly observable.
- The original full serializer remains independent. Diagnostic validation checks
  exact flattened bytes AND reset decisions. Full-baseline mode makes decisions
  from full vectors. Lazy flattening preserves the existing process-local fingerprint
  format; explicit fingerprint calls cannot advance accepted history state. A camera
  owner prevents identity address reuse without retaining a whole previous scene.
- Global generation/upscaler/temporal resets retain the original policy, including
  motion, membership/order, material, lighting and environment changes. No CMake or
  shader contract changes were made. No new GPU wait/drain was introduced.

## Dependency and motion audit

The prior signature includes every collected object's transform and material data,
not only visible shading of one effect. Material/texture/light edits can change
remote GI/reflections/shadows. Transparent histories depend on unique stable
node/submesh correspondence while dense object IDs still affect serialized order.
TemporalAccumulator also checks consecutive sequence, generation, projection,
camera motion thresholds, surface extent/profile, previous position/normal data and
roughness, with native shader disocclusion/coverage rejection. These helpers do not
supply verified moving-object reprojection. Camera view remains outside the global
signature and inside the existing per-history policy; projection/identity changes
retain global resets. No narrower case has visual evidence in this implementation
session, so all previous conservative resets remain enabled. Additional motion
retention and effect-specific reset changes are deliberately deferred.

The reference policy observes captured packed values and native identities, not
untracked in-place writes into otherwise immutable mesh/image contents. Those
operations keep their existing reload/explicit-invalidate requirements. Dirty
revisions with identical effective serialized payload do not invent a new reset.

## Verification delivered and pending

Static source checks cover serializer field/order parity, effective settings getter
coverage, revision lookup, stable correspondence, exact transform comparisons,
independent accepted/fingerprint state, latest-only publication and whitespace.
`tests/draw/temporal_invalidation/probe.hpp` supplies renderer-level source fixtures
for warm frames, camera/projection changes, motion, independent material edits,
effective-cutoff equality/threshold crossing, texture replacement, transparency,
add/remove/reorder, lights, virtual settings, mode switching, explicit reset and
out-of-band fingerprints. It runs both byte and reset validation but does not
render images. No code/shaders were compiled or application/probes executed.

Measure temporal.signatureBytesRebuilt, signatureComponentHits, signatureMaterialHits,
signatureMaterialRebuilds, signatureStateHits, signatureFlattenedBytes and
fullSignatureBytes separately for update and explicit fingerprints. Warm normal
updates should rebuild zero signature payload bytes and avoid full serialization;
diagnostic validation intentionally runs the reference serializer. Capture reset
counts by frame configuration, object motion, material/textures, lights, environment,
geometry/inclusion and membership/order; causes may overlap. No timing reduction
has been measured yet.

Pending exit: execute reference parity and existing temporal/deferred pixel probes
on Vulkan/Metal. Include settings changes while inactive, custom effective getters,
shared native textures with independent source materials, environment/noise changes,
failed/aborted frames, source reload, duplicate/missing correspondence and independent
object round trips. Compare multi-frame convergence and ghosting with full-baseline
mode on camera/object motion and transparent layers. Native allocation/CPU timings,
reset traces and visual evidence are still required for runtime acceptance.
