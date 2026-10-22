# M10 — Final audit of cumulative documentation

[Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Review protocol](../review-protocol.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Entry prerequisites:** Accepted M09-08 documentation closure, final source/product baseline and all M01–M09 documentation handoffs.

M02's accepted review is the source-to-plan record for manipulation identity,
synchronous picking/viewpoint readback, draw overlay and preview lifetime,
partial CPU/live material edits, clone limits and unverified product behavior.
M10 reconciles those contracts with final APIs and maintained docs; it does not
upgrade the M02 continuation assumption into backend-specific runtime evidence.

## Working product

Include [M07 review](../milestone_07_bg2e_composer/review.md) and its eventual
M07-08 coverage record when auditing Composer/root/custom clone/parsed DB/thread/
bake output guidance. The bool SelectableList contract and checked selection
correction survive canonical cleanup; final target identities and package/backend
claims must follow actual M09 source/evidence, not interim app handoff text.

A final reviewed, coherent maintained documentation set under `doc/`, already authored incrementally by M01–M09. It covers the same final API/reference, tutorials, applications/CLI/build/distribution and validation scope originally assigned to M10. This milestone verifies the accumulated result and makes localized corrections rather than writing that set from scratch.

## Scope exclusions

No new engine features, future API guides, missing-feature implementation or deferred bulk documentation authoring. Preserve historical planning evidence separately. No unsupported ResourceManager, native Metal upscaler, DLSS, forward/MSAA or old application compatibility promises.

## Ordered audit steps

| Step | Deliverable |
|---|---|
| [M10-01](step_01_documentation-inventory.md) | Audit cumulative inventory and source/page dispositions |
| [M10-02](step_02_api-reference.md) | Audit final API, capabilities, ownership and extensions |
| [M10-03](step_03_tutorials.md) | Audit delivered-example tutorials and customization |
| [M10-04](step_04_apps-build-distribution.md) | Audit existing app/CLI/build/distribution guidance |
| [M10-05](step_05_documentation-validation.md) | Validate maintained navigation, snippets and consistency |
| [M10-06](step_06_final-review.md) | Final cumulative documentation review and plan closure |

Step IDs and filenames remain stable; their work is now audit/reconciliation of existing content. Substantive missing coverage is an unmet M01–M09 documentation requirement to correct before completion, not a new authoring phase here.

## Exit gate

- Every maintained page, including retained foundations, is reconciled with final code and products.
- All former M10 coverage is accounted for by the [distributed ownership map](../documentation-policy.md); advanced public components remain discoverable and usable.
- API/lifecycle/clone/settings/binding/capability/IO/bake/app/CLI/build guidance is accurate; links, snippets and diagrams are consistent.
- No unimplemented functionality or obsolete production/experimental status is taught as current usage.
- Actual checks and lead evidence remain separate; no agent build/runtime claim without execution authorization.
- review.md records final findings/corrections, scope limits and final acceptance condition. No pending implementation milestones remain.

## M03 reviewed dependency

[M03 technical review](../milestone_03_ray_tracing/review.md) is the actual source/ownership/signal/settings baseline. Its pending-plan impact table and the concrete additions in this milestone's step files supersede historical statements that explicit profiles or effect controls are still unimplemented. Keep exact averaging, chart isolation, required-job capabilities and actual validation evidence distinct from interactive fallback/history behavior.


## M04 reviewed dependency

The [M04 technical review](../milestone_04_lightmaps/review.md) records actual
integrated bake/sample/result ownership, RGB UV2 material semantics, preview
implementation and native shader deployment. Its impact table and concrete
pending-step requirements supersede assumptions that M04 already implements a
headless scheduler, batch writer, full GPU-only resource export or camera-image
averaging. Continuation acceptance does not establish hardware-specific numeric,
performance or exported-file evidence. Preserve production references until M09.


## M05 source-of-truth handoff

At final audit, reconcile the surviving canonical offscreen executable and app docs with post-M09 source. The M05 interim contract includes exact requested camera samples, settings precedence, synchronous typed readback and verified no-clobber PNG output; do not extend that claim to full scene/resource or bake-batch export.

## Current engine preservation requirements — 2026-10-21

Preserve R01–R14 from the source reconciliation: alpha/default/import behavior,
material and native-binding revisions, persistent table/acceleration generations,
completion-aware pools, exact temporal invalidation, conservative transparent
regions, narrow typed update drain elision, CPU/GPU profiling and current complete
save error/publication semantics. Do not reimplement an older version of these
features to satisfy an original milestone description.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-06 actual source/review handoff

[Standalone technical review](../milestone_08_lightmap_generator/review.md) records
implemented public owners, lifetimes/settings/output/CLI contracts and R01–R14
preservation. [Retirement impact and per-file inventory](../milestone_08_lightmap_generator/removal-impact.md)
identify actual transitive shared/header/native/product/template dependencies;
refresh the candidate snapshot before M09 deletion. It is not deletion approval.
The new product's strict required-target failures, native RGB32F and final-manifest
publication are intentional contracts, not old flat-file/skip compatibility.
M08-07 still owns maintained standalone authoring before M09; M10 audits final
M09 names/resources/exports against that authored baseline. Actual native/package/
optimized-counter coverage remains distinguished from synthetic inspection and
lead continuation acceptance. Concrete pending step checklists are updated.

## M08 maintained standalone coverage

The [M08-07 ledger](../milestone_08_lightmap_generator/documentation.md) supplies
actual API/tutorial/app/architecture/DB and navigation coverage, including current
bool-noexcept save behavior and final-manifest/native HDR semantics. Audit its
final post-M09 mappings alongside M01–M09 coverage rather than postponing standalone
authoring. The source review's original gap is closed by delivered documentation,
not transformed into native backend/package/performance evidence.

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

Audit the complete page inventory against the final review contract table, all 33 products and R01–R14. Require M09-08 documentation.md coverage evidence before starting. Preserve foundation/math/json/reflection/input pages after review; retirement is not permission to delete independent documentation.

## Delivered M09-08 audit baseline

Audit [the completed documentation handoff](../milestone_09_cleanup/documentation.md),
documentation-dispositions.tsv and documentation-audit.json. The final set has
229 pages, with a complete supported-product catalog and new production migration,
build/resource, performance and public extension references. Reconcile every
disposition and localized source/wording issue; no bulk authoring is deferred here.
Entry requires M09-08 acceptance under the continuation convention.

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
