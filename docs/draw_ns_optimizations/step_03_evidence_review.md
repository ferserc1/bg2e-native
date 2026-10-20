# Step 03 — Choose the implementation from evidence

Status: Completed. Owner approved indexed GPU shadow UV lookup instead of the reservation experiment.

## Approved replacement decision

The owner rejected further measurement as a prerequisite, requested parity with
production's indexed GPU attribute lookup, approved the presented implementation
steps, and instructed implementation in chat. The reservation-only proposal below
is superseded, not implemented. Both Vulkan and Metal shadows now fetch UV0/UV1
from shared tracing vertex/index buffers. Dedicated per-frame UV triangle packing
and uploads are removed; lightweight shadow material/texture metadata remains to
preserve the existing capacity/fallback policy. See Step 04 for the implementation
and unexecuted validation cases. No production-render or GPU-backend migration is
part of this approval.

## Goal

Turn targeted measurements into an explicit, approved change list. Do not implement
an optimization merely because a broad profiler region has a large total.

## Analysis

### Evidence carried forward from Step 02

- Same-scene Metal shadow-only and combined profiles identify
  `shadowTable.trianglePacking` at approximately 23.4–25.2 ms per scene frame,
  with approximately 380,302 packed triangles per preparation. The inclusive
  `shadowTable.build` cost is approximately 23.5–23.8 ms per scene frame. Alpha
  texture deduplication, metadata/validation and shadow instance-buffer upload
  are each below approximately 0.03 ms per scene frame; triangle-buffer upload is
  approximately 1.0–1.14 ms per scene frame.
- RT-off, GI-only and reflection-only contexts skip shadow-table preparation;
  `shadowTable.skippedNoShadowCapacity` is recorded per tracing preparation.
- Treat triangle UV packing/triangle count as the leading measured scaling
  variable. This establishes where CPU time is spent, not yet which semantics-safe
  optimization to implement. Do not infer that topology/material shortcuts are
  authorized.
- Long `metal.device.waitIdle`/drain intervals coincide with nonzero queued
  renderer updates and are configuration-transition costs. Completion waits also
  occur in stable intervals and remain unattributed to a higher-level initiator.
- The supplied log includes stable intervals with zero pending renderer updates.
  Enabling the profiler from Graphics settings and closing that window is a valid
  capture procedure; an open-window comparison is unnecessary for the shadow
  builder question unless separately investigating UI-originated stalls.
- The profile's `shadowTable.trianglePacking` and `shadowTable.build` regions are
  useful for shadow attribution. A scope-lifetime defect affected outer geometry
  and instance-upload labels and was corrected; a duplicate
  `tracing.shadowTablesAndBuffers` label was also corrected after capture. Do not
  use those contaminated/duplicated outer rows from this capture to estimate
  instance upload or wrapper exclusive time.

1. Normalize timings to scene frames and operations. Use exclusive work regions
   without adding nested inclusive totals. Account for captures crossing a session
   boundary and report substantial profiler overhead if observed.
2. Identify the dominant shadow preparation operation and its scaling variable:
   instances, triangles, unique resources, repeated searches or allocations.
3. Attribute recurring waits and large delegate stalls to their callers. Separate
   required GPU-lifetime synchronization, accidental repeated work, and intentional
   blocking user operations.
4. Estimate a bounded benefit using measured removable CPU work. Do not promise
   equivalent FPS gains or infer GPU execution time from CPU encoding measurements.

## Candidate decisions, not preapproved changes

- If allocations/growth dominate, consider exact sizing, reservations and bulk
  writes within the current frame's preparation.
- If repeated linear searches dominate, consider identity-based lookup while
  preserving deterministic first-seen table indices and overflow behavior.
- If identical geometry-derived work repeats within a frame, consider frame-local
  deduplication with explicit instance-to-range mapping and unchanged shader data.
- If an unnecessary preparation or wait dominates, prove the actual consumer and
  lifetime requirements before proposing a guard or narrower synchronization.
- If work is intrinsic to unchanged per-frame reconstruction, document that fact
  rather than introducing cross-frame caching implicitly.

Do not implement alpha-test-specific shortcuts, cross-frame caching, shader
changes or transparency changes under these candidates. Ask for explicit scope
approval if the only useful next optimization requires one of them.

## Deliverable and gate

Replace the pending proposal below with exact files, algorithm changes, expected
invariants, regression checks and measured justification. Present it to the owner.
Step 04 starts only after approval. If evidence is insufficient, request one
specific additional measurement instead of speculative implementation.

## Required updates on completion

Update this file and `summary.md`, including the approval state. Rewrite Step 04's
implementation checklist to match the accepted findings, not the original guesses.

### Decision record

- Evidence and capture references: Owner's 2026-10-20 chat capture, sessions 1
  (RT off) and 3 (shadows only). Session 3 contains seven consecutive stable
  reports with 18 scene preparations each and zero pending renderer updates,
  from presentations=20870 through 20978. Use these 126 preparations for the
  normalized comparison, not the first interval's 17 preparations/16 frame calls.
- Confirmed bottleneck / scaling variable: In those seven reports, packing costs
  23.432–23.612 ms/scene preparation; inclusive builder cost is 23.572–23.756 ms.
  Each preparation packs 380,302 triangles into 18,254,496 bytes, with 57 instances
  and 48 unique meshes. Packing accounts for approximately 99.4% of builder time,
  but only approximately 20.5% of the 114.030–114.723 ms whole-frame time.
  Instance upload is approximately 0.009–0.012 ms and geometry packing approximately
  0.001 ms: the corrected recapture resolves their earlier scope contamination.
  The duplicated shadow wrapper total is unusable, not evidence of two builds.
- Additional confirmed cost: `effect.shadows.record` totals 81.399–81.865 ms per
  scene preparation over 23 invocations. Exclusive image-view resolution,
  sampler resolution, argument-entry encoding and encoder creation sum to about
  76.4–77.1 ms/frame. This is a separate, larger Metal CPU resource-preparation
  cost, not GPU execution or triangle packing. Do not add the inclusive effect
  total to those exclusive values. It merits a separate proposal after this
  bounded experiment, not an unapproved backend rewrite.
- Proposed changes and affected files: Modify only the vector-instance overload
  of `buildShadowQueryTable` in
  `lib/src/bg2e/draw/pipeline/SoftShadowPass.cpp`. Add a sizing pass that checks
  submesh ranges for alpha-tested instances and accumulates their triangle count
  using checked wide arithmetic. Reserve exactly that many triangle entries and
  `instances.size()` instance entries before the existing append loop. Preserve
  its checked vertex/index reads and its packing algorithm. Keep texture lookup,
  first-seen order and overflow handling unchanged. This removes geometric vector
  growth and relocation from a freshly reconstructed table without cross-frame
  caching, shader changes, or deduplicated triangle ranges. Add bounded sizing
  timing and capacity-growth counters to distinguish the predicted mechanism from
  other packing work. Update profiling documentation with their meaning.
- Preserved invariants and verification cases: For valid input, require identical
  instance records, triangle bytes/order, texture/sampler indices and fallback
  count against the original builder. Check empty and opaque-only inputs, mixed
  materials, repeated meshes, multiple submeshes, both UV sets, invalid indices,
  non-triangular ranges, invalid capacity, capacity overflow and address-space
  overflow. Invalid input must still fail safely (the first reported invalid item
  may differ because validation moves earlier). Keep one collection/TLAS build,
  independent effect usage, shader ABI and frame/GPU resource ownership unchanged.
  Add a CPU equivalence probe using existing example/probe conventions without
  CMake changes; document it separately from checks actually executed.
- Predicted benefit and uncertainty: The measured target is ~23.5 ms of packing,
  not a promised saving. Vector growth is visible in source (`push_back` without
  reserve), but allocation/relocation time is not isolated by the capture. Savings
  may be small or zero; the sizing pass has a cost. The upper bound on removable
  packing work is its measured ~23.5 ms, and this reservation-only experiment
  cannot remove all vertex reads/packing. Triangle counts, uploaded bytes and
  texture counts must remain unchanged; triangle-vector growth during packing
  should become zero. No FPS or GPU-time prediction is justified. If improvement
  is absent, return to review rather than stacking speculative optimizations.
- Wait decision: Stable shadow-only reports have negligible acquisition/slot
  waits and no device drains. The RT-off capture's slot-wait parent contains the
  recurring completion waits, consistent with frame-slot reuse synchronization;
  CPU logs alone do not establish whether it can be narrowed. Keep synchronization
  unchanged. Queued-update drains are attributed to configuration changes; their
  necessity and historical second-long stalls are not established by this capture.
- Rejected approaches / scope questions: Do not claim allocation dominates merely
  because packing dominates. Do not remove checked reads without independent
  evidence. Frame-local mesh deduplication is not selected: 57 instances versus
  48 meshes does not establish repeated identical submesh ranges or their cost.
  No alpha-test shortcut, cross-frame cache, shader/layout/capacity change, reduced
  effect invocation count or wait removal is authorized. Metal binding optimization
  remains a separate candidate with its own approval gate.
- Checks performed: Static inspection of the builder, public table layout and
  plan constraints; numerical normalization of supplied stable reports. No build,
  runtime comparison, image validation or profiler-overhead measurement performed.
- Owner approval: Granted in chat for the indexed GPU lookup replacement above,
  not for the historical reservation experiment.
