# M09-07 — Review M09 final production API and revise documentation plan

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M09 draw-only product evidence.

## Scope and deliverable

- `docs/render_to_draw/plan/milestone_09_cleanup/review.md`
- `docs/render_to_draw/plan/milestone_10_documentation/summary.md`
- `docs/render_to_draw/plan/architecture-decisions.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [docs/render_to_draw/plan/architecture-decisions.md](../architecture-decisions.md).

## Implementation work

1. Record final public class/header/namespace, settings/upscaler extension, lifecycle/clone/IO/bake contracts and canonical executable/source locations.
2. Compare final implementation and deletion manifest with every M10 page/tutorial/build/app task; update paths/names/examples/status rather than documenting tentative names.
3. Record native backend exceptions/seams and removed previous APIs; verify no promised old application compatibility reappeared.
4. Update root plan status and decisions while preserving completed historical step intent. Carry remaining verification issues into explicit doc limitations/follow-up items.

## Verification and exit conditions

- A final implementation-to-documentation mapping and updated M10 steps exist, with all supported products and public component extension points accounted for.
- The engine is labeled production draw only after actual acceptance; documentation work has concrete source anchors.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M10 is the final maintained-doc pass, not a substitute for unfinished removal/product verification.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## Revised final handoff

This technical review is followed by [M09-08 — Cumulative documentation](step_08_documentation.md) in the same milestone. On review delivery, progress points to that documentation step, with normal lead acceptance required. The review does not close the milestone or authorize the following milestone by itself. Reconcile its report with the documentation handoff after that final step.

## Current engine preservation requirements — 2026-10-21

Before declaring retirement complete, compare the KEEP/adapt inventory against
R01–R14 and final source. Record output/counter/baseline evidence, actual native
coverage and remaining measurements separately. Preserve shared optimization and
profiling source/probes even if production entrypoints used related instrumentation.
No reversion to full per-frame rebuilding/all-update drains is an acceptable
implicit cleanup consequence.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-06 final review comparison input

Compare final source against the M08 review and refreshed M09-01 per-file manifest:
actual standalone owner names, final component/umbrella accessors, three app and
camera-offscreen targets, config/data identities, manifest/native HDR model output,
native UI seams and template/resource registrations. Resolve every remove/migrate/
keep disposition; do not report a text-match count as a deleted-symbol proof.
Carry P01–P11/optimization evidence by its real owner/result, with unspecified
coverage unspecified, and update final M09-08/M10 page/source/target mappings.

## M09-05 actual retirement and test contracts

Review [products-distribution.md](products-distribution.md) and the four source
delta ledgers with the original manifest. M09-05 removes 98 legacy example files,
73 GLSL files and two flat compiler frontends. CMake changes preserve paired
aggregate/Metal intermediates/incremental copies and install only authoritative
engine trees; new standalone projects default to deferred draw. Updaters refresh
shaders/helper with headers/library. All app identities and current source bodies
remain unchanged. Reconcile test_02/test_03 fixture-specific ports and intentional
explicit baked-mode/typed UV/indirect-only/alpha-one differences; preserved manual
probes are not automatic or executed test suites. Record actual native/package
evidence from the lead separately from the source audits.

## M09-06 actual source/product gate

[Final product validation](final-product-validation.md) supplies the authoritative
final-products.tsv (33 targets), public-api-evidence.tsv (317 public headers),
umbrella-closure.tsv (11 entry roots/255 reachable headers), source digest and
validation-matrix.tsv (33 source-backed core/optimization/M08 scenarios). Four
delta ledgers reconcile 592 unique paths, including 440 absent paths. Use the
actual source catalog; lexical type/export-marker text is not parsed/qualified
API or rebuilt exported-symbol proof. Non-umbrella backend/explicit headers are
retained APIs, not deletion candidates. Standalone checks are literal template
projection only, not executed creator/configuration/launch.

M09-05 was accepted by continuation; individual backend/scenario/artifact/timing
coverage remains unspecified. M09-06 changed only plan/evidence/audit tooling,
with 1,327 engine/product/profiling files unchanged. Keep native/package/output
round-trip and independent optimization/profiling evidence accurately attributed;
no repeated confirmation or new mandatory logs are required by this handoff.
The next step after this review remains M09-08, not M10.


M09-06 correction input: the lead reported a missing countSceneNodes helper in
scene reference save compilation. See the correction section of final-product-validation.md
and validation-corrections.tsv after the four original ledgers. The neutral recursive
helper is restored; agent source-only scans did not establish compilation success.
Retain this failure/correction attribution when reviewing the final evidence.
