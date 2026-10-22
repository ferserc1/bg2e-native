# M10-03 — Audit delivered-example tutorials and customization guides

[Milestone summary](summary.md) · [Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Prerequisites:** M10-01/M10-02.

## Scope and deliverable

Audit existing maintained `doc/` pages and M01–M09 documentation handoffs against final source, products and distributed coverage. Record findings and localized corrections for the final M10 review. This step does not postpone earlier documentation authoring to M10.

## Audit work

1. Compare existing tutorials to actual delivered deferred/manipulation/lightmaps/offscreen examples, effects/history code and public renderer/pass/subclass/shader/upscaler customization call sites.
2. Check no-RT/SDK-unavailable fallback, matched shader folders/indices, settings round trips, headless precedence, UV/sample/time/color semantics and clone independence.
3. Confirm referenced targets/resources exist and obsolete forward/render tutorials are retired or clearly historical; retain valid current GPU tutorials.
4. Correct local snippet/link drift. Do not create the tutorial collection here: missing guides remain obligations of the milestone that delivered the functionality.

For manipulation tutorials, compare examples against the [M02 review](../milestone_02_manipulation/review.md): selection capture and UI event routing, exact/group/regex partial patches, clone behavior, optional final overlays, `--no-rt`, and synchronous best-view limitations. Do not imply the draw utility alone handles arbitrary reflected scene edits, alpha-aware picking or async viewpoint analysis.

## Verification and exit

Existing tutorials describe supported delivered products and match their actual source; execution evidence remains honestly attributed.

Perform relevant static checks. Compilation/runtime validation belongs to the project lead unless explicitly requested. Deliver one step, update progress and wait for lead acceptance under the execution protocol. No new engine feature, build-system change or future API documentation is authorized by this audit.


## M04-07 reviewed implementation requirement

Compare the lightmap tutorial with draw_lightmaps's actual synchronous pre-acquisition sample submission, all-target adoption, frozen settings/source snapshots, cancellation and preview rebinding. Do not describe the example as a headless scheduler, async service, image exporter or GPU-rasterized UV diagnostic.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).

## Current engine preservation requirements — 2026-10-21

Keep current effective cutoff and glTF import examples, independent editable
objects with derived-cache reuse, retired-slot/recording completion and custom-pass
fallbacks. Queued generic callbacks still drain; do not illustrate them as the
specialized no-drain color path. Timing examples must use asynchronous snapshots
and distinguish CPU, GPU, scene cadence and presentation cadence; never sum nested
inclusive scopes or overlapping submissions as total frame time.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-06 standalone tutorial comparison

Audit M08-07's real standalone model/prefab/context example against final source:
explicit live Engine lifetime, retained Scene/root, UV2 before realization, release
previous baker, final settings before preparation, no source material adoption
between targets and output manifest resolution. Keep camera exact-N offscreen and
UV accumulation examples separate. Show real mode/format/model-copy limits and
controlled pixel/geometry reload checks; no unimplemented native capability or
future compatibility recipe may replace the delivered tutorial.

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

Audit examples/draw/01..13 and examples/gpu targets using final-products.tsv, not old numbered legacy tutorials. Cover no-RT behavior, portable upscaler fallback, paired native custom shaders and metadata-backed settings. Baking uses one live target/context and a global submission serial; camera PNG offscreen rendering is a separate workflow.

## M10-01 inventory handoff

Use [inventory.md](inventory.md) and [product-coverage.tsv](product-coverage.tsv)
for current tutorial/product destinations. Applications moved to apps/model_edit,
apps/bg2e_composer and apps/lightmap_generator; their targets drop the draw_ prefix.
Numbered draw/GPU example targets are unchanged. Historical M09 paths remain
evidence only. Tutorial semantics/snippet verification remain this step's work.

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

## Source correction delivered — 2026-10-22

The user subsequently authorized source repair. Both findings are corrected in
[source-corrections.md](source-corrections.md): GPU 11/13 serialize shared UBO access,
and all four standalone helpers check the canonical paired shader paths. Earlier
finding/handoff paragraphs and stored audit hashes above are historical snapshots,
not current open issues. Native validation and final lead acceptance remain pending.
