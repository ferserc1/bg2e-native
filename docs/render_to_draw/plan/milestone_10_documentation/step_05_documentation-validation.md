# M10-05 — Audit links, snippets and maintained-document consistency

[Milestone summary](summary.md) · [Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Prerequisites:** M10-01–M10-04.

## Scope and deliverable

Audit existing maintained `doc/` pages and M01–M09 documentation handoffs against final source, products and distributed coverage. Record findings and localized corrections for the final M10 review. This step does not postpone earlier documentation authoring to M10.

## Audit work

1. Validate maintained navigation/relative links/anchors and symbol/header/target/resource references across all inventory entries, including pages kept unchanged.
2. Search for obsolete render/scene::vk/native callbacks, aliases/target paths, experimental-opt-in claims, forward/MSAA/clone promises and inconsistent diagrams/shader names. Distinguish deliberate historical discussion from working usage instructions.
3. Compare snippets statically with delivered code; builds/runtime demonstrations require explicit lead authorization. Record unexecuted snippets and unspecified backend coverage honestly.
4. Apply localized consistency corrections and resolve all audit findings; this is an audit of previously authored pages, not deferred creation of the documentation set.

## Verification and exit

All maintained inventory entries have resolved audit dispositions; no broken navigation or obsolete working-code guidance remains.

Perform relevant static checks. Compilation/runtime validation belongs to the project lead unless explicitly requested. Deliver one step, update progress and wait for lead acceptance under the execution protocol. No new engine feature, build-system change or future API documentation is authorized by this audit.

## M03-10 reviewed implementation requirement

Check that visible-window/app initialization snippets actually connect/bind/draw settings, with rebind/cleanup and current defaults. Link actual lead evidence separately from source/static checks; continuation acceptance is not hardware-specific numeric/performance proof. Confirm all M03 documentation obligations were authored during M03-11, not deferred to this audit.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.

## Current engine preservation requirements — 2026-10-21

Check code/snippets/links for current defaults, v3/v5 bindings, revision and
completion APIs, optimization diagnostic switches, Performance/recording controls
and bool writer return handling. No old cutoff 0.5 default, unconditional TLAS
rebuild, all-queued-update drain or detached worker recipe may remain as current
usage merely because it appeared in an earlier accepted milestone report.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-06 consistency searches

Search maintained guidance for stale standalone-not-implemented text, tentative
SceneAssembler/Batch names, root umbrella export assumptions, flat-file output,
byte-clamped float claims, skipped-required-target success, interchangeable camera/
UV samples and old production callback examples. Keep explicitly historical
production descriptions distinct from final usage. Verify read-only artifact
inspection is not described as full geometry reload or native/performance proof.

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

Check all delivered links, names, snippets, diagrams and shader paths against current source and the 317-header evidence catalog. Separate syntax/source checks from executed examples. Preserve exact capacity 16384, bindings and interface versions where documented; never infer performance measurements from retained profiling probes.

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

## Source correction delivered — 2026-10-22

The user subsequently authorized source repair. Both findings are corrected in
[source-corrections.md](source-corrections.md): GPU 11/13 serialize shared UBO access,
and all four standalone helpers check the canonical paired shader paths. Earlier
finding/handoff paragraphs and stored audit hashes above are historical snapshots,
not current open issues. Native validation and final lead acceptance remain pending.
