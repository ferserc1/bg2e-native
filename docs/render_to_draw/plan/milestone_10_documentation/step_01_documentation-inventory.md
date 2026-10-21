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

## M09-06 final source-to-documentation evidence

[Final product validation](../milestone_09_cleanup/final-product-validation.md)
and its header/product/umbrella catalogs identify actual source paths and targets
for the documentation audit. The 33-scenario matrix maps core, native UI, app/CLI,
M08 P01-P11 and optimization/profiling evidence; per-scenario native coverage is
not specified by continuation acceptance. Do not turn source-only audits into
runtime/exported-symbol or configured-standalone claims. M09-08 must author the
missing maintained usage updates before M10 audits completeness.

## Final M09-07 source baseline and handoff

Use [the final review](../milestone_09_cleanup/review.md) and its
[page inventory](../milestone_09_cleanup/documentation-handoff.tsv). M09-06 and
its scene/Metal/VMA corrections are accepted by continuation. Production is
`bg2e::draw`; GPU is its low-level backend abstraction. Retired render APIs and
old application targets have no compatibility promise. The source catalog lists
33 products; native scenario/backend/performance coverage remains unspecified.
M09-08 authors the complete cumulative maintained documentation before M10.
M10 audits that delivered documentation and makes localized corrections; do not
defer missing API, tutorial, build or application authoring to this milestone.

Reconcile every documentation-handoff.tsv row with the M09-08 delivered disposition, destination and source anchor. Inventory new pages and deleted-page incoming links as well. Report uncovered contracts instead of silently accepting a smaller page set.

## Delivered M09-08 audit baseline

Audit [the completed documentation handoff](../milestone_09_cleanup/documentation.md),
documentation-dispositions.tsv and documentation-audit.json. The final set has
229 pages, with a complete supported-product catalog and new production migration,
build/resource, performance and public extension references. Reconcile every
disposition and localized source/wording issue; no bulk authoring is deferred here.
Entry requires M09-08 acceptance under the continuation convention.
