# M10-06 — Final cumulative documentation review

[Progress](../progress.md) · [Requirements](../requirements-map.md) ·
[Documentation policy](../documentation-policy.md) · [Milestone](summary.md)

## Baseline and decision

Reviewed 2026-10-22. Starting and finishing HEAD:
`e2a32164a781dbf420a4464b25ddce9feb862c6a`, with inherited, uncommitted M10-01–05
documentation and audit artifacts preserved. The initial review changed only plan evidence. The later user-requested correction
changes two example sources, four executable scripts and affected documentation;
no commit, staging, engine/shader or CMake changes were made. Linux is the agent
workspace; SDK, driver, GPU and lead-tested backend/scenario details are not
specified. No new native validation is inferred from the workspace platform.

M10-05 is accepted by project lead continuation instruction. This final review is
**delivered, awaiting final lead acceptance**. There is no successor milestone.
Documentation audit delivery and the subsequently requested source corrections are
complete. The examples now serialize shared UBO access and all four standalone
scripts check canonical shader paths. Final lead validation remains pending;
no native execution result is inferred. See [correction evidence](source-corrections.md).

Production is `bg2e::draw`, with low-level Vulkan/Metal implementation in
`bg2e::gpu`. Retired `bg2e::render`, old app identities and compatibility aliases
are not supported. Later app relocation is reconciled against current source:
`model_edit`, `bg2e_composer`, `lightmap_generator`; the surviving camera CLI is
`draw_offscreen_application`. The [33-product catalog](product-coverage.tsv)
records exact example/app targets, directories, registrations and guides.

## Requirements and cumulative evidence

[final-requirement-coverage.tsv](final-requirement-coverage.tsv) accounts for all
43 decisions in the requirements map, their original owning steps, current
contract/source/page evidence and classification. Staged dual-path requirements
are historical through M08 and intentionally terminate in M09 retirement; they
are not missing compatibility overloads today. The ledger is traceability to
reviewed contracts, not a new independent compiler or exhaustive per-symbol proof.

All ten rows of the documentation policy's distributed coverage map are accounted
for as follows. Historical handoffs and hashes retain their original revisions.

| Required coverage | Delivered final evidence and outcome |
|---|---|
| Every page and disposition, including foundations | M10-01 page-inventory.tsv: 241 historical dispositions, 229 live pages; M10-05 validation-pages.tsv reconciles all live final hashes. |
| Public API, namespaces, lifecycle and ownership | M10-02 api-contracts.tsv: 37 contract families across 55 pages and 115 source anchors; header-coverage.tsv maps 317 public headers at discovery depth. |
| Live catalogs, settings, clone and edits | SettingsCatalog/codec/facade, retained owner/rebinding, deep clone, partial stored/live edits and coordinated replacement documented in M10-02; headless/app precedence reviewed in M10-04. |
| Native capabilities, bindings and customization | GPU capability versus SDK/effect support, explicit native v3/v5 interfaces, public compatible pass/shader injection and external renderer/upscaler hooks reviewed in M10-02/03. |
| Delivered tutorials | M10-03 maps 31 pages and 26 numbered examples; 426 blocks classified, including full source assemblies for draw 12/13. GPU 11/13 now explicitly serialize shared UBO access. |
| Shader/fallback/sample/clone limits | M10-02/03 separates no-RT/SDK fallback, exact UV/camera samples from display histories, explicit signal/color/clone limits and resource ownership. |
| Apps, CLI, IO and async lifecycle | M10-04 maps 18 contract families/17 guides, 38 parser option rows and current app/CLI identities; M10-05 repairs worker/token/safe-update descriptions. |
| Build/resources/distribution/retirement | M10-04 deployment-shaders.tsv derives 292 output paths and records platform generators/archive contracts; the four standalone preflights now use the canonical paired bake path. |
| Navigation/snippets/diagrams/retained pages | M10-05: 229 pages reachable from doc/index.md, 1,913 local links/anchors, 111 includes, 30 exact source excerpts and six statically reviewed diagrams. |
| Actual-code/product review and limits | This report, final-requirement-coverage.tsv, final-preservation-coverage.tsv and final-audit.json; final lead acceptance remains pending. |

The [handoff ledger](handoff-coverage.tsv) reconciles 216 explicit historical page
mappings from all nine M01–M09 documentation reports. Its destinations and reports
exist. M09-08 authored the cumulative set before M10; no bulk authoring requirement
was deferred into this review. M08 standalone output/context/assembly/batch guidance
is mapped to final bake, DB, tutorial, app and architecture pages. Final page
hashes match M10-05, including unchanged math/JSON/reflection/input foundations.
No further maintained-page correction was necessary in M10-06.

## Planned versus actual contracts and decisions

| Contract | Final source/document conclusion | Classification |
|---|---|---|
| Renderer/delegate and frame | Runtime-selected exclusive renderer, retained shared injected components, borrowed frame bindings and explicit linear/final boundary. No template-locked public renderer. | Equivalent lifecycle refinement. |
| Settings and extensions | Live accessor catalogs are authoritative; reflected identity/version/validation, optional automatic persistence and headless precedence are explicit. Custom components/shaders must satisfy format/binding/ownership contracts. | Equivalent refinement; no universal arbitrary-shader compatibility. |
| Scene/clone/DB | Root/schema/reflection/factory contracts are separate. Deep CPU/GPU clone retains independence and registered custom clone limits. Parsed CPU loading and stopped GPU realization differ; complete bool-noexcept saves require checked failure. | Equivalent refinement; schema-only output is not complete GPU resource export. |
| Manipulation/UI | Typed weak selection, synchronous stopped picking/readback, independent stored/live material edits and overlay/preview teardown remain high-level. Async workers are joined; queued mutations validate owners and quiesce producers. | Equivalent refinement. |
| Effects/bake/offscreen | Public query/effect/native interfaces, explicit signals, display histories and exact UV/camera accumulators remain distinct. Bake owns jobs/pixels/output; geo owns UV2 generation; batch stages complete outputs and publishes manifest last. | Equivalent refinement; no implicit full-batch atomicity/durability. |
| Production retirement | Native Vulkan stays in GPU; scene retains neutral visitors. Surviving draw/gpu public API and canonical apps replace the deleted render API and old products. | Intentional contract change required by M09. |
| Distribution | All four standalone helpers check the canonical paired bake output path, including shell macOS Metal checks. | Source correction delivered; native execution remains lead-owned. |
| Primitive GPU tutorial samples | Device waitIdle completes previous readers before shared UBO writes. GPU 13 advances samples only after successful acquisition; GPU 11 also guards null acquisition. | Simple serialized example policy; overlapping-frame optimization is not required. |
| Native/performance acceptance | Earlier continuation acceptance is preserved with unspecified scenarios; no new backend/output/timing matrix supplied here. | Verification gap, not an invented success or source regression. |

Generic BlueNoise remains in utils; environment/cubemap/IBL/BRDF remains public
under draw::environment; advanced passes remain under draw::pipeline. GPU native
interop is the permitted FSR seam. Reduced internal resolution without FSR remains
the default. ResourceManager redesign, native Metal upscaler, DLSS, forward/MSAA,
new refit API and old-product compatibility remain excluded. Existing private cache
helpers are justified ownership/cache machinery, not an obligation to expose them
or replace them with duplicate public objects.

## R01–R14 preservation

[final-preservation-coverage.tsv](final-preservation-coverage.tsv) closes each
preservation item at source/document contract depth, retaining its actual source
anchors and maintained guides. This does not close independent performance or
native acceptance work.

| IDs | Preserved behavior |
|---|---|
| R01–R02 | Authored alpha default 0, effective threshold 0.02; normalized glTF MASK cutoff 0.5; ForceOpaque/nonopaque classification and blended exclusion. |
| R03–R04 | Material/texture/native revisions, no-op realization reuse, identity-indexed immutable tables and transactional capacity admission. |
| R05–R06 | Tracing v3/shadows v5; exact device-clamped capacity up to 16,384; reserves AO (4,2,13), GI (5,3,15), reflections (6,3,16), shadows (2,1,10). Vulkan effects scene set1/material set2 bindings5/6; shadow invocation set0/scene set1/alpha set2 bindings16/24. Metal effect records8/9, shadow records5/6, retained native owners/residency. |
| R07–R08 | Exactly matching normally completed recording/BLAS generation permits acceleration reuse, without refit. Consumer leases and completion gate pooled allocations; budgets never evict live work. |
| R09–R11 | Exact temporal invalidation/manual unobserved-edit invalidation, conservative regional raster/composition fallbacks, full effect domains and narrow typed final-color drain elision. Generic/resource edits remain conservative; GPU idleness does not stop CPU producers. |
| R12 | Independent live collection and JSONL recording; original timing identity/status, asynchronous collection and no measurement waits. Vulkan pipeline boundaries differ from Metal encoder envelopes; closing Performance does not stop recording. |
| R13–R14 | Checked bool-noexcept complete saves, staged/no-clobber resource publication and main-file/manifest-last policy; actual integrated and standalone public bake infrastructure supersedes historical pending descriptions. |

Independent optimization/profiling reports retain their own provenance and pending
measurements. Migration continuation and source existence are not benchmarks.

## Verification and style

| Evidence owner | Actual result and limitation |
|---|---|
| Agent final static reconciliation | 43 decisions, R01–R14, nine handoff reports, 317 headers, 33 products and 229 final page hashes accounted for. Revalidated 373 stored source-hash occurrences covering 350 distinct paths; zero drift/errors. See final-audit.json and final-source-hashes.json. |
| Agent maintained-document audit | Fresh final-documentation-audit.json: 1,913 links/anchors and 111 includes, zero errors. Two MyComponent includes are explicit user-defined reflection examples. M10-05 exact-excerpt/JSON/fence/reachability checks remain valid because maintained pages did not change. |
| Agent source spot checks | Rechecked current alpha/opacity declarations, bool-noexcept IO signatures, paired shader path derivation and all four obsolete script checks, and primitive UBO writes before frame acquisition. These corroborate the inherited audits; no native failure was reproduced. |
| Lead acceptance | M01–M09 and M10-01–05 accepted through the recorded continuation convention. Earlier macOS arm64 scene/Metal/VMA compiler/linker corrections are historical reported issues subsequently accepted. Progress also records lead-reported validation through M02-01 on macOS Metal and Linux Vulkan, and the M01 HDR correction. Later backend/scenario/SDK details are unspecified; those earlier reports do not certify later source. |
| Synthetic M08 evidence | Ten synthetic inspector tests remain historical inspection evidence; they are not native GPU, package or complete output validation. |
| Not executed | Configure/build, shader/compiler/test/native app/HTTP, package/install/standalone creator/updater, image/output inspection, diagram rendering and performance capture. |

The initial review changed only Markdown and audit ledgers. The subsequent source
correction preserves the full GPL headers in both examples, adds braced control
flow and changes no namespace/include/function/method-group layout. New block
braces use separate lines. No engine/shader or vendored file was modified. Shell
syntax, isolated preflight cases, source ordering and working-tree whitespace
checks pass; no build or native application was run.

## Findings and pending-plan impact

| Finding | Actual evidence / owner | Impact and disposition |
|---|---|---|
| Shared UBO correction | GPU 11 waits before writing lightUbo; GPU 13 waits before acquisition/camera writes and shared accumulation use. | Implemented at user request; tutorial snippets explain deliberate frame serialization. No outstanding source finding. |
| Standalone preflight correction | Both shell/PowerShell creators and updaters use bake/uv_surface/surface.frag.spv; shell macOS checks use the matching .metallib. | Implemented at user request; canonical/missing/obsolete-only shell fixture cases checked without executing build/copy scripts. No outstanding source finding. |
| V01 — unspecified native/package/performance coverage | No new named backend, output archive, image/counter or performance report supplied. | Lead-owned verification remains separate; no reconfirmation requested and no fabricated result. |
| Final handoff | M10-06 has no implementation successor. | Updated summary, final step, root index, requirements map and progress. Final lead acceptance pending; source corrections await lead validation. |

No missing bulk API/tutorial/app documentation was identified at the recorded audit
depths. Both subsequently authorized source corrections are delivered and no longer
listed as open issues. No redundant milestone or future-API guide is added.

The edited step file is step_06_final-review.md. Earlier M10 audit artifacts remain
historical snapshots; source-corrections.md and its ledgers supersede their original
source findings and hashes for the changed paths. Final acceptance is owned by the
project lead. Progress remains awaiting_project_lead with no implementation successor.

## GPU 13 native failure correction — 2026-10-22

The lead subsequently reported a copyImage format/extent validation abort. The
[display correction](rt-display-correction.md) replaces the RGBA-to-BGRA copy with
a surface-format fullscreen pass, paired shaders, normal RGB order, Metal compute
closure and resize rebinding. Tutorial updated; no engine/backend/CMake changes.
Compilation/native confirmation remains pending; rebuild the example/new shaders.
Earlier correction checks describe their original source snapshot.
