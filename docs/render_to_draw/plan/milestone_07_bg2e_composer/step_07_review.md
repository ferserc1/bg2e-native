# M07-07 — Review M07 scene/import contracts and revise CLI/cleanup

Consume [M07-06 product validation](product-validation.md), including the Chain
control parity correction, source-backed workflow/distribution matrix and explicit
runtime/package evidence gaps. Do not convert static target discovery into native
launch proof or defer lost workflows to M09.

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M07 product/scene/bake evidence.

## Scope and deliverable

- `docs/render_to_draw/plan/milestone_07_bg2e_composer/review.md`
- `docs/render_to_draw/plan/milestone_08_lightmap_generator/summary.md`
- `docs/render_to_draw/plan/milestone_09_cleanup/summary.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

## Implementation work

1. Record actual graph/factory/clone/IO/resource identity, async-safe publication, widgets and scene-wide bake semantics.
2. Compare prefab/context root behavior, target naming and complete saves with M08 scene assembly/batch output; update pending steps rather than assume old APIs still fit.
3. Audit remaining production references in shared libraries/apps/scripts/tests and revise M09 removal inventory and M10 documentation mapping.
4. Update all M08–M10 summaries/steps for accepted changes and verified/unverified backend/application scenarios.

## Verification and exit conditions

- The review names actual interfaces and edits all impacted pending tasks, with complete-scene/bake evidence.
- No hidden production dependency is being justified as harmless merely because the new app window works.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M08 runs against the now-complete shared scene/resource APIs; final deletion remains gated on standalone product parity.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## Revised final handoff

This technical review is followed by [M07-08 — Cumulative documentation](step_08_documentation.md) in the same milestone. On review delivery, progress points to that documentation step, with normal lead acceptance required. The review does not close the milestone or authorize the following milestone by itself. Reconcile its report with the documentation handoff after that final step.


## M07-04 implemented import boundary

Review [async-import-service.md](async-import-service.md) against actual joined
worker/HTTP lifecycle, publication rollback and same-renderer generation refresh.
Compare CPU-only GltfDocument/ObjMesh/Bg2Mesh realization overloads with M08 parser
reuse, and include app-local private httplib headers/settings plus retained legacy
server paths in M09 retirement. Actual native HTTP/backend evidence remains a
lead-owned check; static inspection does not establish it.
