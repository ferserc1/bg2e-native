# Revision-keyed temporal signatures

RendererDeferred reuses immutable signature components for frame configuration,
geometry/correspondence, object transforms, effective material/resources, lights,
environment and effect settings. The source RenderQueue remains an independently
owned scene extraction; no renderer material-table index becomes a stable identity.

Effective material identity and captured values/bindings/classification revisions
come from the phase 02 pack cache. The material lookup also captures native image/
sampler identities and realization revisions. One component serves repeated
references to the same captured source/revision, without merging independent
materials by value. Geometry keys retain DrawMesh/BLAS owners and node/submesh
correspondence. Exact world-matrix bits retain full snapshot and fingerprint identity. A separate history-compatibility comparison omits only those transform bytes.

The warm path checks ordered correspondence, source identities, effective revisions,
world matrices and small global dependencies. When unchanged, it returns the same
component graph without allocating per-object maps or serializing material payloads,
transforms or settings again. A settings/light-only edit shares every instance
component. Dirty frames rebuild only changed payload components and current lookup
maps. Caches retain only the latest derived collection plus the renderer's accepted
history state; removed source owners are dropped when those states are replaced.
Camera identity retains its component without retaining an entire previous scene.

Settings have mutable controls and injected virtual getters, so they are checked
using typed effective snapshots rather than unsafe setter-only revision counters.
Only explicit signature fields are compared: padding is ignored for settings,
while float bits retain the full reference's exact equality semantics. Public light
and environment fields remain observable without requiring a new authored revision
API. Source mesh/native buffer immutability and explicit invalidation obligations
for untracked in-place content writes remain the same as the reference path.

The interactive global invalidation policy separates rigid motion from other edits:

- Transform-only object motion preserves global generation and upscaler state.
  Motion is observed using node/submesh correspondence and temporarily permits
  rolling accumulation; it does not implement object motion vectors. Full frozen
  offscreen fingerprints still change on transforms and reject mutation.
- Extraction-order/membership changes, geometry/source identity,
  packed material or texture identity, lights, environment, projection/extent,
  indirect mode and effective settings retain their existing global reset behavior.
- Camera view is excluded from the global scene signature as before; the temporal
  accumulator still performs its existing camera threshold, projection, sequence,
  previous-surface and disocclusion checks. Fingerprints include view separately.
- Transparent node/submesh correspondence must remain unique/nonempty whenever
  temporal effects require it. Dense object IDs/order remain observable in the
  serialized reference, but do not define the stable correspondence identity.
- Material/light edits can change distant GI/reflections/shadows. No local-only or
  effect-specific reset is introduced without native convergence/ghosting evidence.
  Temporal generation and upscaler resets still advance together for global causes.

At a synchronized update boundary:

```cpp
renderer.setTemporalSignatureValidationEnabled(true); // Exact bytes + reset parity.
renderer.setTemporalSignatureReuseEnabled(false);      // Independent reduced reset-policy reference.
```

The full serializer is retained independently. Validation throws on a mismatch of
full component bytes, reduced policy bytes, or their respective change decisions.
Baseline mode uses an independent serialization with only world matrices omitted
for reset decisions. Full and reduced reference baselines have separate storage. Switching modes does
not itself invalidate histories. Derived-cache/fingerprint queries never replace
the accepted history state, so an out-of-band fingerprint cannot consume an edit.
`invalidateHistories()` remains the explicit global-reset API.

`sceneFingerprint()` preserves the existing process-local byte format, including
view and history generation. The component graph flattens lazily and caches that
vector for explicit fingerprints/diagnostics; the vector-returning API still copies
its result. Normal renderer updates do not flatten. Fingerprints are runtime job
consistency guards, not serialized material IDs or portable object fingerprints.

Profiler counters include temporal.signatureBytesRebuilt, signatureComponentHits,
signatureMaterialHits/Rebuilds, signatureGeometryHits, signatureTransformHits,
signatureCameraHits, signatureLightHits, signatureEnvironmentHits, signatureSettingsHits,
signatureStateHits, signatureFlattenedBytes and fullSignatureBytes (all with the
`temporal.` prefix). Reset counters include signatureResets, explicitOrLifecycleResets
and reset.initialSignature/frameConfiguration/membershipOrOrder/geometryOrInclusion/
materialOrTextures/lights/environment/settings. Transform-only edits increment
objectMotionObservations instead of signatureResets or reset.objectMotion. Causes can overlap;
do not sum them as independent resets. Separate update and explicit fingerprint
calls when measuring hits/bytes; validation intentionally restores full serializer
work. Native timings and visual acceptance remain pending.

Phase 0 accepts trails on moving receivers and stale AO/GI/reflections on stationary
receivers, including temporal-upscaler artifacts when only camera/depth vectors are available. Motion renews a per-key
rolling window lasting 3*maxFrames subsequent rendered scene records; it preserves
the authored freeze preference. This is approximate stale-lighting recovery, not
exact local invalidation. Existing application callbacks that explicitly invalidate
or reattach scenes can still reset; these are separate integration concerns. Native
visual validation and performance measurements remain pending.
