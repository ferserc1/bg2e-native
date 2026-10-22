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

## M10-01 current inventory handoff — 2026-10-22

Use [inventory.md](inventory.md), [page-inventory.tsv](page-inventory.tsv),
[header-coverage.tsv](header-coverage.tsv) and
[product-coverage.tsv](product-coverage.tsv) for the current source/page baseline
at HEAD e2a32164a781dbf420a4464b25ddce9feb862c6a. All 229 maintained pages,
317 public headers and 33 products have inventory owners. Headers are mapped at
module discovery depth; per-symbol semantics still require the API audit.
Applications now live directly under apps/ with targets model_edit,
bg2e_composer and lightmap_generator. Historical M09 product paths/hashes are
preserved evidence, superseded for current usage by this inventory. Twenty-one
pages changed with the later app relocation; M10-01 corrects two additional stale
DB-writer availability descriptions. No substantive authoring gap was identified
at inventory depth; semantic/native/performance verification is not inferred.

## M10-02 API audit handoff — 2026-10-22

Consume [api-reference-audit.md](api-reference-audit.md) and
[api-contracts.tsv](api-contracts.tsv) alongside the original inventory. Current
reference corrections cover retired API/writer/preview availability, live/stored
materials, actual catalog identities and persistence opt-in, raw bake filtering,
borrowed GUI ownership and completed acceleration reuse without refit. Native
v3/v5 capacity/reserve/binding contracts and R01–R14 remain preserved. Draw API
navigation now links the existing optimization/profiling references directly.
M10-01 page hashes remain historical; api-page-changes.tsv records the subsequent
18-page delta. No build/runtime/native/performance evidence is added. Retain the
separate tutorial, CLI/distribution and global validation scopes of M10-03–05.

## M10-03 tutorial audit handoff — 2026-10-22

Use [tutorials-audit.md](tutorials-audit.md), tutorial-coverage.tsv and
tutorial-snippets.tsv for all 31 tutorial pages/26 numbered products. Draw 06
now describes EnvironmentGenerator/SkyboxRenderer instead of the old manual
conversion recipe. Other corrections cover Model Edit target spelling, GLSL
binding syntax, acquired-slot retirement, borrowed resource retention, history
invalidation and actual capability/customization behavior. Tutorials 12/13 still
reconstruct their sources ignoring whitespace. No CMake/source/native execution.

Preserve the explicit source follow-up: primitive GPU 11/13 samples share mutable
light/camera UBOs without a proof that all readers are complete before host writes.
The tutorial wording now states that limitation; sample binaries are unchanged.
M10-06 must record the overlap risk separately from completed documentation and
unspecified native acceptance. Whole-document validation/package/build audits
must not claim it was repaired by documentation or by continuation alone.

## M10-04 application/distribution audit handoff — 2026-10-22

Use [apps-distribution-audit.md](apps-distribution-audit.md),
apps-distribution-contracts.tsv, cli-option-coverage.tsv and deployment-shaders.tsv
for current app/CLI/package source contracts. Ten documents are corrected,
including scripts/PACKAGING.md. Linux emits tar.gz with dirty/overwrite options;
macOS/Windows emit ZIP without those switches. HTTP guidance now describes the
current Window menu, app-owned joined parser and stopped publication. Composer
Performance ownership is explicit. The canonical lightmap CLI is not a second
preserved production application; the standalone template uses deferred draw.

A new source finding is unresolved: all four standalone creator/updater scripts
check bake/uv_surface.frag.spv, but the paired compiler emits
bake/uv_surface/surface.frag.spv (same Metal mismatch on macOS). Fresh canonical
builds fail preflight; rebuilding alone cannot fix it. This is an unmet M09-05
script integration follow-up, documented without executable-script/CMake changes.
Final review must retain this blocker and the inherited GPU 11/13 shared-UBO risk
separately from documentation completion; neither was repaired by acceptance or
wording. No package/creator/updater/native/HTTP/output execution is claimed.

## M10-05 delivery handoff — 2026-10-22

[Whole-document validation](documentation-validation.md) covers all 229 maintained
pages, root reachability, 1,913 local links/anchors and 111 includes. Sixteen pages
corrected; 30 exact excerpts match current source and six diagrams reviewed
statically. Separate validation ledgers preserve earlier audit snapshots. No
implementation/CMake changes or build/runtime execution. M10-05 awaits acceptance;
M10-06 must retain both unrepaired source follow-ups (primitive shared mutable UBOs
and standalone shader preflight mismatch) separately from documentation completion.

## M10-06 final delivery — 2026-10-22

[Final review](review.md), final-requirement-coverage.tsv and
final-preservation-coverage.tsv reconcile the full requirement/preservation map,
all nine handoffs and current source/products. M10-05 accepted by continuation;
M10-06 awaits final lead acceptance. No successor milestone or implementation
step remains. Documentation audit and user-requested source corrections are delivered;
GPU 11/13 serialize shared UBO writes and all standalone checks use canonical
shader paths. Both corrections await lead validation. No build/runtime or
new performance evidence. Earlier handoff status paragraphs remain historical.

## Source correction delivered — 2026-10-22

The user subsequently authorized source repair. Both findings are corrected in
[source-corrections.md](source-corrections.md): GPU 11/13 serialize shared UBO access,
and all four standalone helpers check the canonical paired shader paths. Earlier
finding/handoff paragraphs and stored audit hashes above are historical snapshots,
not current open issues. Native validation and final lead acceptance remain pending.

## GPU 13 native failure correction — 2026-10-22

The lead subsequently reported a copyImage format/extent validation abort. The
[display correction](rt-display-correction.md) replaces the RGBA-to-BGRA copy with
a surface-format fullscreen pass, paired shaders, normal RGB order, Metal compute
closure and resize rebinding. Tutorial updated; no engine/backend/CMake changes.
Compilation/native confirmation remains pending; rebuild the example/new shaders.
Earlier correction checks describe their original source snapshot.
