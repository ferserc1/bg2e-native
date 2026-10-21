# M10-01 — Audit the accumulated documentation inventory

[Milestone summary](summary.md) · [Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Prerequisites:** Accepted M09-08 documentation closure.

## Scope and deliverable

Audit existing maintained `doc/` pages and M01–M09 documentation handoffs against final source, products and distributed coverage. Record findings and localized corrections for the final M10 review. This step does not postpone earlier documentation authoring to M10.

## Audit work

1. Collect M01–M09 documentation.md coverage/source/disposition records and their current maintained doc/ pages; do not restart authoring from the initial plan.
2. Check that every maintained page, including retained foundation pages, has a resolved keep/update/remove/replace disposition and final source owner. Confirm docs/render_to_draw history is separated from maintained usage guidance.
3. Compare the inventory with final M09 source/public headers, targets and settings schemas; identify omissions against the distributed owner in documentation-policy.md. Make localized inventory corrections; substantive missing content must be repaired as an unmet owning-step requirement.

## Verification and exit

Complete traceable inventory of existing cumulative documentation, with no unowned omissions.

Perform relevant static checks. Compilation/runtime validation belongs to the project lead unless explicitly requested. Deliver one step, update progress and wait for lead acceptance under the execution protocol. No new engine feature, build-system change or future API documentation is authorized by this audit.

## Current engine preservation requirements — 2026-10-21

Include maintained TemporaryResourcePool, TemporalInvalidation, TransparentRegions,
UpdateSynchronization, TracingSceneResources and cpu_profiling references, plus
Performance UI/CPU/GPU transport/recording ownership. Link optimization/profiling
implementation reports as evidence with their own validation state; their historical
planning inventories are not current API descriptions.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-06 specific coverage audit

Inventory M08-07 documentation.md and the maintained standalone bake API,
model/prefab/context tutorial, new CLI/app, generic DB file/archive and deployment
pages. M08 review found the old bake index falsely claimed scheduling was absent;
verify that its owning documentation step corrected it. Track final M09 page/path
changes from actual source rather than treating the historical removal TSV as a
document inventory. Missing substantive standalone authoring remains an M08-07 gap.

## M08-07 authored coverage available

Use [documentation.md](../milestone_08_lightmap_generator/documentation.md) and its
31-page source/disposition mapping. The public bake index, real standalone API
references, generic DB file/archive, tutorial, full CLI and architecture pages are
now authored. Final audit checks their post-M09 names/links/semantics and retained
foundations; do not treat the earlier review's authoring gap as still absent or
recreate parallel future guides. Native and synthetic evidence remain distinct.
