# M08-06 — Review M08 and prepare exhaustive production removal

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M08 and all prior product gates.

## Scope and deliverable

- `docs/render_to_draw/plan/milestone_08_lightmap_generator/review.md`
- `docs/render_to_draw/plan/milestone_09_cleanup/summary.md`
- `docs/render_to_draw/plan/milestone_10_documentation/summary.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

## Implementation work

1. Record actual standalone states/generations, bake effect configuration, scene assembly, pixel/output/overwrite contracts and CLI identities.
2. Compare every planned legacy removal with current engine/app/example/test/script/shader references; update M09 to an actual removal/replacement manifest.
3. Compare public namespaces/types and executable deployment with M10 API/tutorial/app documentation paths. Revise pending M09/M10 steps and root decisions.
4. Explicitly inventory remaining required parity/verification gaps before declaring the old API removable; preserve reference source until that gate is accepted.

## Verification and exit conditions

- The report shows all requested products and a concrete remaining-dependency/removal impact table.
- Cleanup and documentation steps reference real new APIs/targets, including specialized output resources in bake.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M09 is a deliberate compatibility-breaking retirement milestone, not an opportunity to retain permanent hidden render shims.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## Revised final handoff

This technical review is followed by [M08-07 — Cumulative documentation](step_07_documentation.md) in the same milestone. On review delivery, progress points to that documentation step, with normal lead acceptance required. The review does not close the milestone or authorize the following milestone by itself. Reconcile its report with the documentation handoff after that final step.

## Current engine preservation requirements — 2026-10-21

Compare R01–R14 explicitly with the delivered standalone scheduler/assembler/batch
and all M09–M10 owners. Check fresh wrapper versus reusable immutable resources,
completion-safe cancellation, source/material identity, bool export failures and
separate workload waits from asynchronous timing. Preserve renderer/private cache
helpers and independent workstream validation provenance; no duplicate cache,
profiler or generic ResourceManager is required by M08.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-05 actual validation handoff

Use [product-validation.md](product-validation.md) as the source parity and
runtime evidence ledger. Independent PNG/native-map/material-section inspection
is implemented and its ten synthetic CPU tests pass; it does not establish
native geometry reload, actual bake numerics/chart dilation, source preservation,
process cancellation, package relocation or Vulkan/Metal performance. Preserve
P01–P11's explicit lead-owned status and the accepted continuation policy without
inventing scenario/backend results. Carry strict required-target failure,
manifest-based atomic publication, native RGB32F format and settings-identity
changes into the actual M09 feature/dependency comparison. Existing production
reference/native optimizations remain unchanged; this step supplies no deletion
authorization or blanket product-retirement success.
