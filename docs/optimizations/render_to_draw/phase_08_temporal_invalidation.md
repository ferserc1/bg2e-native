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
