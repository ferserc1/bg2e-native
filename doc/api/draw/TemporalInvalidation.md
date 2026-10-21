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
correspondence. Exact world-matrix bits retain conservative object-motion resets.

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

This phase deliberately preserves the previous global invalidation policy:

- Object motion, extraction-order/membership changes, geometry/source identity,
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
renderer.setTemporalSignatureReuseEnabled(false);      // Full serialization baseline.
```

The full serializer is retained independently. Validation throws on a mismatch of
component bytes or reset decisions. Baseline mode uses the full serialized vector
for decisions, retaining its previous reference while active. Switching modes does
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
objectMotion/materialOrTextures/lights/environment/settings. Causes can overlap;
do not sum them as independent resets. Separate update and explicit fingerprint
calls when measuring hits/bytes; validation intentionally restores full serializer
work. Native timings and visual acceptance remain pending.
