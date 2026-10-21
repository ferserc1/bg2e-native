# M10-06 — Review cumulative documentation and close the plan

[Milestone summary](summary.md) · [Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Prerequisites:** M10-01–M10-05.

## Scope and deliverable

Audit existing maintained `doc/` pages and M01–M09 documentation handoffs against final source, products and distributed coverage. Record findings and localized corrections for the final M10 review. This step does not postpone earlier documentation authoring to M10.

## Audit work

1. Create review.md only during execution with actual source/revision/product baseline, cumulative M01–M09 documentation evidence and M10 audit findings/corrections.
2. Compare final documentation and delivered products against every preserved coverage requirement in documentation-policy.md and requirements-map.md; verify source/API/ownership/lifecycle/settings/CLI/build/extension correctness.
3. Confirm no unimplemented planned feature has been described as available. Resolve in-scope omissions against their owning documentation steps; list only explicit out-of-scope follow-ups and truthful verification limits.
4. Update progress to final acceptance pending after delivery. Lead acceptance closes the plan; do not report completion merely because the audit was planned.

## Verification and exit

Complete final audit report and coherent maintained documentation; final lead acceptance closes the implementation plan.

Perform relevant static checks. Compilation/runtime validation belongs to the project lead unless explicitly requested. Deliver one step, update progress and wait for lead acceptance under the execution protocol. No new engine feature, build-system change or future API documentation is authorized by this audit.

## Current engine preservation requirements — 2026-10-21

Explicitly close R01–R14 against final implementation and maintained guidance.
Preserve independent optimization/profiling acceptance provenance and outstanding
performance measurements; code existence or migration continuation is not a new
benchmark/native matrix. Final migration retirement must retain optimized behavior,
fall back safely and keep independent object import/export semantics.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-06 closure evidence input

Reconcile final standalone products/data contracts and all M08-07/M09-08 maintained
page mappings against final source, recording actual M09 retirement dispositions
and current optimization/profiling preservation. Separate ten synthetic inspector
tests, continuation acceptance and any later supplied native output/package/counter
records. No unknown backend/scenario should become a fabricated successful run;
no missed standalone authoring requirement is deferred beyond final closure.

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

Close only after every M09-08 page disposition, final public extension contract, 33-product mapping and R01–R14 coverage is accounted for. Record localized audit fixes and unresolved lead-owned native/package/performance coverage without inventing results. Any missing bulk documentation remains an explicit unmet M09-08 requirement.
