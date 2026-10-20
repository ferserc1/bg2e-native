# Experimental draw namespace CPU optimization plan

Status: Indexed GPU shadow lookup implemented with owner approval; native build/runtime validation pending.

## Objective

Identify and reduce avoidable CPU work in the experimental `bg2e::draw` renderer,
using measurements supplied by the project owner. Preserve visual behavior and
the production renderer's per-frame scene-table/TLAS reconstruction strategy so
performance comparisons remain meaningful.

## Starting evidence

The Metal captures supplied in the conversation show:

| Measurement | Before scene-binding reuse | After scene-binding reuse |
|---|---:|---:|
| Scene resource-set creations per scene frame | 46 | 2 |
| Scene-binding preparation | About 136 ms | About 6.2 ms |
| `renderer.recordLinear` | About 142 ms | About 11.7 ms |
| Stable `frame.total` | About 177–192 ms | About 42–48 ms |

The scene still executes 23 deferred compositions per scene frame. Its TLAS is
built once. Repeated scene-binding requests now reuse snapshot-local resource
sets keyed by layout identity, set index and effective capacity. This correction
is already implemented; it is not a future plan step.

The remaining dominant measured CPU region is `tracing.shadowTablesAndBuffers`:
about 24 ms total and 23 ms exclusive per scene frame. Its internal cause has not
yet been isolated. Some captures also contain 10–13 ms completion waits and
isolated 1.2–1.6 second stalls in `frame.prepareDelegate`. Neither observation
establishes its underlying cause.

`tracing.allocateAndUploadInstanceBuffer` currently includes the nested shadow
preparation interval. Its inclusive time must not be interpreted as an expensive
instance-buffer upload. CPU TLAS timings measure encoding, not GPU execution.

## Scope and constraints

- Modify experimental draw code and narrowly necessary supporting diagnostics.
  Production `bg2e::render` is a reference, not a refactoring target.
- Preserve one per-frame scene collection, scene-table generation and TLAS build.
  Retain the existing immutable geometry cache and snapshot-local binding cache.
- Do not introduce cross-frame material/table caching, dirty flags, material
  revision tracking or TLAS refit in this plan. Those require a separate proposal.
- Do not change alpha-test semantics, transparent ordering, composition count,
  shader algorithms, effect quality or ray/sample budgets. Alpha-test-specific
  optimizations require explicit owner approval before adding an implementation step.
- Do not modify CMake or compile without explicit owner authorization.
- Preserve unrelated working-tree changes. Follow repository license/style rules.
- Keep all generated files and code comments in English.
- Activate diagnostics through Graphics settings, not environment variables or
  command-line options. Write logs through the existing home-directory/appId path.
- Keep `gpu` independent of higher-level engine namespaces. Shared diagnostics
  belong in an allowed foundation module; resolve application identity above it.

## Execution order

| Step | File | Purpose | Owner capture required |
|---|---|---|---|
| 01 | [Baseline](step_01_baseline.md) | Establish reproducible comparison and inspect current code | Yes |
| 02 | [Targeted instrumentation](step_02_instrumentation.md) | Isolate shadow CPU cost, waits and delegate stalls | Yes |
| 03 | [Evidence review](step_03_evidence_review.md) | Select and approve narrowly scoped implementation | No new capture unless evidence is insufficient |
| 04 | [Measured optimization](step_04_optimization.md) | Implement the approved CPU changes only | Yes |
| 05 | [Validation and comparison](step_05_validation.md) | Check behavior, lifetime safety and production comparison | Yes |

Execute sequentially. Steps 01 and 02 are investigation steps, not permission to
optimize. Pause after requesting each owner capture; never invent measurements.
Steps 03 and 04 may repeat only for a newly identified, documented bottleneck and
with owner approval. Do not expand scope automatically.

## Mandatory plan maintenance

After every step, update its Markdown file **and this summary** before proceeding:

1. Set status to `In progress`, `Waiting for owner profile`, `Completed`, or `Blocked`.
2. Record files changed, observations, measurements, capture conditions and log
   references. Distinguish observed facts from hypotheses and predictions.
3. Record checks actually performed; explicitly identify uncompiled/unexecuted work.
4. Document decisions, rejected approaches, remaining questions and scope impacts.
5. Update later steps when discoveries invalidate assumptions. Explain the reason
   and obtain approval for scope expansion; do not silently rewrite the objective.
6. Add a dated entry to the execution record below and update the results table.

For each optimization, compare the same scene/settings/build/backend and report
per-scene-frame costs, creation counts and whole-frame distributions. Do not sum
nested inclusive timings or compare modal-dialog stalls to steady-state rendering.

## Results

| Step | Status | Evidence / decision |
|---|---|---|
| 01 | In progress | Same-scene Metal effect variants show shadow preparation ~23–25 ms/scene frame when shadows are on and negligible when off. Build/validation settings and reference image remain unreported. |
| 02 | Completed | Owner profiles isolate ~23.4–25.2 ms/frame to shadow triangle UV packing (~380,302 triangles/preparation). Disabled shadows skip table prep. Queued-setting drains are attributed; low-level completion waits remain unexplained. |
| 03 | Completed | Owner approved indexed GPU UV lookup in place of the reservation experiment. |
| 04 | Completed implementation | Removed shadow UV triangle packing/uploads; both native shaders read shared geometry. Build/runtime validation pending. |
| 05 | Planned | Depends on implementation and owner verification |

## Execution record

- Owner approved and requested indexed GPU UV lookup for shadows, superseding
  the reservation experiment. Implemented Vulkan/Metal lookup from existing
  immutable tracing geometry, retained shadow-specific alpha metadata/fallback,
  unified standalone preparation through tracing snapshots, removed UV triangle
  storage/uploads and obsolete profiling. Updated probes and documentation. New
  shadow kernel/layout ABI is v3 and requires regenerated shaders. No production
  render/backend/CMake changes or compilation/runtime execution.

- 2026-10-20 — Step 03 evidence review completed; owner approval pending.
  Seven stable shadow-only reports (126 scene preparations) isolate
  23.432–23.612 ms/preparation in triangle packing. Proposed checked sizing and
  exact vector reservation as a bounded experiment; allocation's contribution is
  unknown. Separately identified ~76.4–77.1 ms/frame in exclusive Metal resource
  preparation stages during shadow recording; deferred to a separate proposal.
  Updated Step 04's proposed checklist. No optimization, compilation or runtime
  validation performed.

- Planning only: created this plan from the supplied profiles. No code changes,
  builds, profiling runs or runtime validation performed as part of plan creation.
- 2026-10-20 — Step 01 investigation updated from owner-provided Metal profiler
  output. Confirmed snapshot-local binding reuse in source and recorded stable
  RT-enabled measurements, including approximately 23–25 ms/frame for
  `tracing.shadowTablesAndBuffers`. The agent incorrectly interpreted repeated
  looking pasted blocks as duplicate capture data; the owner clarified that the
  profile file was cleared before each capture. No source changes, builds,
  profiling runs or runtime validation were performed by the agent.
- 2026-10-20 — Improved CPU profiling session identification: log session markers
  now include a monotonically increasing session ID and local/Unix start
  timestamps, and Graphics settings show the current session metadata plus a
  reminder to label capture configurations when sharing. Updated profiler guidance
  for capturing/sharing each configuration. This is diagnostic-only and does not
  change rendering behavior. No build or runtime validation performed; compilation
  was not requested.
- 2026-10-20 — Added automatic draw report-boundary context: backend, surface and
  internal extents, presentation/scene-frame counters, renderer family and active
  deferred RT effect switches. The owner confirms one scene remains fixed across
  variants; the scene API does not need to provide a display name. Updated profiling
  documentation. No compilation or runtime checks performed.
- 2026-10-20 — Owner clarified that all captures used the same scene and that the
  pasted profiles represent distinct effect variants; repeated text in the first
  paste led the agent to misattribute blocks. Context-tagged reports now show
  shadow table preparation at ~23–25 ms/frame when shadows are enabled and about
  0.007–0.008 ms/frame when disabled. No bottleneck cause beyond this region is
  established; Step 02 remains diagnostic-only.
- 2026-10-20 — Corrected effect context logging: deferred RT effects now report
  actual enabled settings as well as pass availability. Added explicit diagnostics
  accessors for AO, GI and reflections. Re-recorded baseline interpretation from
  owner clarification; the scene is fixed across variants and does not need a
  scene-name field. No compilation/runtime validation performed.
- 2026-10-20 — Implemented Step 02 instrumentation only: non-overlapping shadow
  builder regions, corrected instance-upload boundary, separate shadow buffer
  uploads, bounded table/count/byte metrics, consumer counts and delegate/wait
  initiator labels. `git diff --check` passed. The project was not compiled or
  run as instructed. Waiting for owner captures before evidence review.
- 2026-10-20 — Step 02 completed using owner-supplied profiles. The isolated
  triangle-packing scope accounts for ~23.4–25.2 ms/frame for ~380,302 triangles
  per shadow-enabled preparation; other builder regions and uploads are small.
  Disabled-shadow contexts skip shadow table preparation. Long device drains
  coincide with queued renderer updates. Fixed duplicate wrapper-label accounting
  discovered during review. Low-level queue completion waits in stable intervals
  remain unattributed and are carried to Step 03 as an evidence limitation. No
  optimization implemented or approved.
- 2026-10-20 — Reviewed the owner's full Step 02 log. It includes multiple
  steady-state effect variants, with `shadowTable.trianglePacking` at
  ~23.4–25.2 ms/frame for ~380,302 triangles per shadow-enabled preparation.
  Queued renderer update drains identify configuration-transition intervals.
  The owner supplied profiles captured with the profiler enabled then its window
  closed; this is valid, and no UI-open capture is needed for shadow attribution.
  Fixed a duplicate wrapper timing label after the capture; direct builder/packing
  measurements are valid. Low-level queue completion waits remain unattributed.
  No build or runtime validation performed.
