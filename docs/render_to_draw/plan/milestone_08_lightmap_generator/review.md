# M08 technical review — Standalone draw/bake product and retirement handoff

[Plan](../index.md) · [Progress](../progress.md) · [Product validation](product-validation.md) ·
[Retirement impact](removal-impact.md) · [M08-07 documentation](step_07_documentation.md)

## 1. Baseline, authority and product status

Review date: 2026-10-21. Starting and finishing HEAD:
`78943d5820e69363c942d71136f479a0df87128e`; no commit is created by this review.
All inherited uncommitted M07 documentation/reconciliation and M08 implementation
are retained. The review covers delivered M08-01–05 sources and current engine
optimizations, not only tracked changes versus HEAD. This step adds review and
candidate-inventory evidence and revises pending plan files; it does not alter
engine/application/shader/CMake behavior or delete production code.

The tool host reports Darwin. No SDK version, device/driver/capability, native
runtime/package or performance query was made. M08-01–05 are accepted by project
lead continuation instructions under the existing execution protocol. The latest
request accepts M08-05; compilation/checks are assumed successful, with backend
and scenario details **not specified**. Ten synthetic Python inspector tests
passed in M08-05; they did not run the engine. Review-time checks are static only.

Current production remains bg2e::render through M08. The implemented alternative
is draw + public bake, with three apps and required supporting examples. M09 is
the deliberate compatibility-breaking promotion/removal milestone; M08-07 must
first close cumulative maintained documentation. This report neither starts M09
nor introduces an extra acceptance/reconfirmation flow.

| Requested product | Actual current target/source | Status and evidence |
|---|---|---|
| Deferred/no-RT, query shadows, advanced tracing/history/denoise and public component construction | draw_deferred_renderer, examples/draw/10_deferred_renderer; draw/pipeline | Delivered earlier; accepted by continuation, specific native/counter coverage unspecified. |
| FSR/external upscaler extension and reduced-resolution path | Public draw renderer/upscaler and deferred example | Existing implementation retained; no new Metal native upscaler/DLSS or SDK success claim. |
| Manipulation, material patches, independent GPU clones, picking/viewpoint | draw_manipulation, examples/draw/11_manipulation; manipulation/utils | Earlier accepted product; synchronous readback limits retained. |
| Integrated maps, UV diagnostics/previews and material adoption | draw_lightmaps, examples/draw/12_lightmaps; bake integrated APIs | Separate from standalone batch/file transaction; earlier accepted product. |
| Exact camera sample offscreen export | draw_offscreen_application, examples/draw/13_offscreen_application | Earlier accepted headless product; camera samples are not UV samples. |
| Model editor | draw_model_edit, apps/draw/model_edit | Workspace/UV2/best-view/material/resource-save product retained; backend/package coverage unspecified. |
| Composer | draw_bg2e_composer, apps/draw/bg2e_composer | Scene/import/integrated batch product retained; lead's explicit selection correction is distinct from broad HTTP/package coverage. |
| Standalone lightmap generator | draw_lightmap_generator, apps/draw/lightmap_generator | M08 delivered; model/prefab/context, typed config, UV jobs and complete-generation output. P01–P11 coverage is recorded separately. |

## 2. Planned versus actual public contracts

The source anchors below, their implementations and real app callers were checked
alongside the [context](standalone-context.md), [assembly](scene-assembly.md),
[batch](batch-output.md) and [CLI](cli-application.md) handoffs.

| Object/area | Actual contract and ownership | Plan disposition |
|---|---|---|
| bake::StandaloneBakerContext / StandaloneLightmapBaker | Public dedicated headers; context borrows initialized windowless draw Engine/Device, retains scene/native owners and one serial slot. Created → Initialized → SceneReady → Cleaned. | Equivalent refinement of the public standalone owner; no extra Engine/camera renderer. |
| Initialization/preparation | initialize(shaderRoot, BakePassConfiguration); prepareScene(loaded shared root, optional EnvironmentResources). Null uses public authored/default environment generation. Failed/repeated preparation retires previous generation and leaves Initialized. | Delivered public injection on context. Batch/CLI exposes only compatible public LightmapSettings, not arbitrary camera effect profiles. |
| Target state/generations | createBaker with target/shared settings, frozen at reset; one live target must release before next. Ready/Running/Complete/Cancelled/Invalidated/Failed; stale scene cannot reset into another scene. Engine identity, queue/revisions, BLAS and environment generations checked. | Required generation/lifetime contract delivered; stopped producers/current validity still required for unobservable external writes. |
| Sample execution | update owns exact internal recording/slot/serial, immediateSubmit and RecordingCompletion; zero-count TLAS preflight, one completed valid update per sample, composition/dilation/readback before Complete. record rejects external/repeated commands. | Equivalent synchronous scheduling, not camera averaging. Fresh wrapper may share immutable tables/completed acceleration; no forced full native rebuild. |
| Accumulation/result | Public LightmapBakeExecutor/UvTemporalAccumulator/LightmapCompositionPass; raw signal/counts, chart validity, completed current result and independent retained output owners. | Existing M04 public algorithms reused; no copied AO/GI/temporal/denoise implementation. |
| bake::StandaloneBakeSceneAssembler | Actual name replaces tentative SceneAssembler. BG2/VWGLB/OBJ CPU readers, context scene and single/multi-root prefab schema, source-only context, enabled authored target list, helper exclusion, staged independent UV2 before target upload; optional deferred realization. | Equivalent naming/preparation refinement. Retain Scene alongside root. Unknown schema types follow shared parser warnings/skip; registered retained custom components require complete resource contracts. |
| Assembly identity/dependencies | Target node, inputSourcePath, stable hierarchy/sibling outputIdentity, UV validation/generated metadata; scene/targetRoot and known canonical sourcePaths. Lexical paths preserve relative-file semantics. | Known paths alone are not protection proof; batch supplements original OBJ/MTLs and declared custom inputs. No pick/table/global editable material ID. |
| bake::StandaloneBakeBatch | Static run(Engine&, Assembly&, output, Options, callback), copied/frozen options, loaded/custom hook ownership check, CPU/uploaded target agreement, complete output preflight, serial baker release, no source material adoption. | Delivered scheduler. Malformed required targets reject whole job; this intentionally differs from legacy per-target skips/zero-success. |
| bake::LightmapOutputWriter | preflight all identities/protected files/namespaces; staged write once per target, validateSources, commit only complete current batch; overwrite policy frozen. | Specialized owner is bake. Old DB production writer remains only for coexistence and is removed after its callers retire. |
| Publication | Private stage + cooperative directory lock; fresh complete generation then authoritative lightmaps.json atomically last. Old generations retained on overwrite; manifest-relative paths. | Intentional stronger transaction/output-layout contract. No simultaneous multi-path replacement, hostile-process security or power-loss durability promise. |
| Typed pixels/metadata | RGB8 selected PNG/JPEG/BMP/TGA with no second exposure; lossless decode/byte checks, explicit lossy JPEG. RGB32F exact native RGBA32F/alpha one, no exposure/gamma/clamp/65504 cap. Linear/top-row-v=0/UV2/multiplier/mode/settings metadata. | Actual native float archive format, not arbitrary interchange HDR; draw loader required. Capability/compute validation may reject invalid results rather than clamp. |
| Optional model copies | Detached independent CPU/GPU drawable clone, staged actual map, linear AO-slot bakedAO/bakedGI, UV2/scale one/channel zero; complete engine-aware DB native snapshot, bool-noexcept false checked. | Real resource export, not sampler-only adoption. Single model copies omit node placement/descendants/unrelated components; no full prefab export claim. |
| Generic DB additions | CPU TextureArchive overload with explicit mipLevels/minMipSize zero, original GPU overload unchanged; publishFile with native no-replace/explicit replacement; OBJ sourcePaths original OBJ/read MTL files. | Useful generic file/parsed-data functionality remains DB after production retirement. |
| CLI/settings | draw_lightmap_generator model/prefab; typed draw OffscreenApplication delegate, 1×1 depthless context, no camera frames, finish runs batch; all native scene/job owners release before Engine cleanup. | Delivered standalone product, not a standalone renderer variant. Existing production CLI unchanged. |
| Config identity/precedence | SettingsCodec schema bg2e.draw.settings/version 1, renderer bg2e.bake.standalone.v1, component bake/type bg2e.bake.lightmapSettings.v1; actual live settings bound, import before GPU, explicit CLI overrides last. | Equivalent public codec use. Unknown/unavailable values reject or receive validated skip warnings; renderer/type/schema mismatches reject, no preferences/display profile relabelling. |
| Interrupt/errors | Signal handler stores request only; callbacks/current check before work/after completed sample/before commit; cancelled count zero/no new paths. Exit 0 success/help, 2 args/config, 1 runtime/output/cleanup, 130/143 cooperative SIGINT/SIGTERM. | Stricter final-sample/later-target cancellation than legacy partial publication. Interrupt after commit does not undo output; cleanup failure can report failure after a complete commit. |
| Current public exports | bake/all.hpp explicitly exposes implemented owners; root bg2e/all.hpp still includes render and does not include bake/all.hpp. Shared app/scene/DB/UI headers still expose production branches. | Coexistence, not zero transitive dependencies. M09 must migrate actual umbrellas/accessors; do not claim the final canonical export already exists. |

Actual declaration anchors:
[StandaloneBakerContext](../../../../lib/include/bg2e/bake/StandaloneBakerContext.hpp),
[StandaloneLightmapBaker](../../../../lib/include/bg2e/bake/StandaloneLightmapBaker.hpp),
[assembler](../../../../lib/include/bg2e/bake/StandaloneBakeSceneAssembler.hpp),
[batch](../../../../lib/include/bg2e/bake/StandaloneBakeBatch.hpp),
[writer](../../../../lib/include/bg2e/bake/LightmapOutputWriter.hpp),
[generic file IO](../../../../lib/include/bg2e/db/file.hpp),
[archive](../../../../lib/include/bg2e/db/texture_archive.hpp),
[CLI parser](../../../../apps/draw/lightmap_generator/src/CommandLine.cpp),
[CLI owner](../../../../apps/draw/lightmap_generator/src/main.cpp).

## 3. Completed scope, missing work and exclusions

M08-01–05 implementation/product validation preparation is delivered and accepted
by continuation. No missing required source implementation was established by
this review. The stronger whole-batch output and stricter required-target success
semantics are intentional recorded contract changes, not hidden legacy parity.

Current omissions belong to explicit owners:

- **M08-07 maintained documentation:** doc/api/bake/index.md still says standalone
  scheduling is unimplemented. Add actual standalone/assembly/batch/output
  reference/tutorial/app guidance, reconcile LightmapOutput/DB writer pages and
  navigation, and record cumulative coverage. This is not deferred to M10.
- **M09 canonical cleanup:** transitive production headers, dual accessors/modes,
  native UI/application seams, legacy apps/examples/tests/shaders/templates and
  umbrella exports remain. They are required planned cleanup, not M08 deletion.
- **Verification coverage:** actual native artifacts, geometry reload, known chart
  dilation/numerics, process cancellation/faults, relocated packages and optimized
  counters have no named backend/scenario evidence supplied. Lead continuation
  accepts the delivered step under protocol without inventing those details.

Deliberately excluded: general ResourceManager/global editable material catalog,
new graphics effect implementations, asynchronous staging/readback, parallel
standalone target jobs, arbitrary custom-component resource discovery, prefab
scene export via model copies, automatic generation garbage collection/crash lock
recovery, hostile filesystem races or power-loss durability. The inspector only
supports bounded PNG/native map files and BG2 material sections, not full geometry
reload or all byte codecs. No new DLSS/native Metal upscaler/forward/MSAA product
is promised. No permanent old application API/ABI compatibility survives M09.

## 4. R01–R14 current-code preservation check

The [source reconciliation](../code-reconciliation-2026-10-21.md) is authoritative;
its original absent-standalone snapshot is history, now superseded by M08 code.
No source performance improvement is claimed from this review.

| ID | Current implementation checked / M08 disposition | Pending preservation owner |
|---|---|---|
| R01 | Material default cutoff 0/effective threshold 0.02, glTF MASK policy; assembly/clone/output retains authored semantics. No restoration of historical default 0.5. | M09-03/06, M10-02/05 |
| R02 | ForceOpaque for effective noncutout, ForceNonOpaque for cutout; transparent exclusion remains in public collector. | M09-04/06, M10-02 |
| R03 | Value/binding/classification revisions, native texture/image generations and completed-slot reuse remain; source checks observe them and cloned adoption updates clones only. | M09-03/06, M10-02 |
| R04 | Indexed device-clamped tables and immutable sharing reused by executor; no global material identity or per-target duplicate editable catalog. | M09-03/04/06, M10-02 |
| R05 | Current tracing v3/shadows v5 ABI/reserves/capacity and paired stages remain untouched; no old v2/256 ceiling reinstated. | M09-04/05/06, M10-02/04 |
| R06 | Persistent tuple/native array owners and residency stay inside existing services; retaining snapshot/output owners is not cache eviction permission. | M09-04/06, M10-02 |
| R07 | Exact recording completion/BLAS generations checked; zero-sample preflight may prime completed reuse. Fresh wrapper is not a demand for a fresh TLAS allocation. | M09-04/06, M10-02 |
| R08 | Completion-aware pools/logical leases remain; standalone output/accumulation lifetime is independent of camera history. No native pool reset/reimplementation here. | M09-03/04/06, M10-02 |
| R09 | Renderer lazy temporal signature cache remains. Stopped standalone queue/file/geometry validation is a separate correctness workload, not a replacement for optimized per-frame signatures. No batch throughput gain is claimed. | M09-03/06, M10-02/04 |
| R10 | Transparent-region conservative bounds/fallbacks remain in renderer. UV/effect receiver and tracing domains are not assumed region-limited. | M09-04/06, M10-02/03 |
| R11 | Typed final-color drain elision remains narrow; bake resource preparation/clone/readback/cleanup synchronizes stopped work as required, without changing live renderer update semantics. | M09-02/03/06, M10-02 |
| R12 | Base CPU/GPU telemetry, bounded async timing and Composer Performance UI remain. No new timing waits, per-sample flushes or recording controls are added. | M09-02/04/06/08, M10-02/04 |
| R13 | Complete DB save bool-noexcept result is checked explicitly before target success; native resource no-clobber/copy fallback/main-last logic is retained under staged model directories. Batch final manifest supplies its separate transaction. | M09-03/05/06, M10-02/04 |
| R14 | Public bake primitives reused and actual standalone classes/CLI now exist; new source anchors replace pending-owner assumptions. Existing optimized private helpers are retained, not made obsolete by namespace cleanup. | M08-07, M09-01/03/05, M10-01/02 |

## 5. Verification and code style

| Evidence class | What is established | Limits |
|---|---|---|
| Agent source review | Public owners/fields/state paths, exact command completion, serial source/target validity, assembly roots/hooks/UV2, typed output/preflight/clone/export/manifest order, CLI/catalog/cleanup and current resource registrations. | Static inspection, not compilation or native numeric/output proof. |
| Agent dependency snapshot | 1338 candidate source/build/resource files, 316 direct textual legacy matches; per-file disposition/owner plus concrete incoming replacement table. | Comments/string matches and whole-tree candidates included; not a semantic/transitive graph or deletion approval. Refresh in M09-01. |
| Agent source style | 25 M08-owned/modified C/C++ files: opening full GPL, resolved project includes, balanced lexical delimiters and braces/new-line placement for control flow checked; namespace indentation, function separation, class method-purpose grouping and necessary private quoted includes reviewed. No violations requiring edits found. | Third-party files not rewritten. No shader was changed by M08. Static/structural checks are not a compiler/style-parser guarantee. |
| Earlier synthetic tests | M08-05 reports ten passing standard-library inspector tests on temporary CPU files, including HDR100000/PNG filters/orientation/native material sidecars/errors. | Not rerun here; no actual engine output, BG2 geometry reload or backend/package result. |
| Lead continuation | M08-01–05 accepted by project lead continuation instruction; compilation/checks assumed successful. | Backend/scenario/platform/SDK/artifact details not specified. M07 selection correction retains its distinct explicit provenance. |
| Native product/regression | P01–P11 in product-validation.md, batch/CLI/standalone/assembly and optimized/profiling probes define lead checks. | No new native artifacts, process fault/lock tests, measured counters or relocated package evidence supplied. |
| Review artifact checks | Local links/paths/fences, inventory path existence/uniqueness/disposition totals/hash, Python syntax and git diff --check. | No CMake configure/build, app/native probe, engine test or package operation. |

Source-style audit scope is the dedicated standalone public/implementation files,
new generic DB file pair, modified DB texture/OBJ headers/sources and three
umbrellas/contracts, app parser/header/main and four M08 probe headers.
Full-style preservation applies to future touched files even when the removal
inventory itself reports an old reference; no bulk cosmetic rewrite is justified.

## 6. Pending-plan impact and architecture decisions

[removal-impact.md](removal-impact.md) and its TSV are the concrete current-source
input to M09. Do not delete shared files wholesale or native gpu implementations
based on a broad render/native text match. The existing app/export/config names
are current facts, not tentative aliases. Any future canonical rename is made
once in M09 with callers, build/package/template/config/documentation checked.

| Actual finding / decision | Concrete pending edits made |
|---|---|
| Delivered public owners and explicit include, no camera executor | M08-07 requires actual API/reference/tutorial/app coverage and fixes stale bake index; M09-02/03 handles shared app/umbrella/accessor migration. |
| Source-backed replacement inventory rather than deleting render directory first | M09-01 refreshes per-file TSV and incoming consumers; M09-03 separates obsolete writer/scene implementation from neutral file/parsed-data APIs; M09-04 relocates retained native UI/app operations. |
| Three surviving app targets/config identities, two distinct offscreen workloads | M09-02 records canonical naming migration without accidental profile relabelling; M09-05 updates actual registrations, templates, probes and paired resources. |
| Complete-generation manifest/native RGB32F/bool-checked models | M09-03/05/06 preserve generic archive/publish and bake writer semantics; M10-02/04 audit actual final IO/material/CLI schemas and paths. |
| Explicit P01–P11/evidence limits and optimized source safeguards | M09-06 repeats real products/optimized baselines; M09-07 carries final evidence; M09-08 documents actual final contracts. M10-05/06 must not convert synthetic/continuation acceptance into hardware claims. |
| Maintained documentation still lacks standalone reference | M08-07 owns authoring now; M10-01 inventories this coverage, M10-02/03/04 audit final public/API/tutorial/app guidance, not bulk authoring. |
| Interim production/experimental state still real | Root index/decisions/source anchors/requirements map and M09/M10 summaries link this report; M09-08/M10 remove obsolete usage only after actual cleanup. |

Edited owning step files: M08-07; M09-01,02,03,04,05,06,07,08; M10-01,02,03,04,05,06.
The review updates their work/exit focus, not completed history or implementation
scope. Architecture choices remain: production through M08; draw/bake/gpu public
separation; no ResourceManager; data readability separate from old source/ABI;
no permanent forwarding shim after M09. This review introduces no user-decision
change or extra approval checkpoint.

## 7. Open issues and transition

No native implementation failure was reported or established by this review.
Known pending work is the explicit M08-07 maintained documentation gap and the
M09 cleanup/transitive/native/API inventory. Actual product/backend/package and
optimized-counter coverage remains unspecified, governed by lead acceptance and
P01–P11, not silently promoted to proven scenarios.

Deliver this review awaiting lead validation. A continuation accepts M08-06 and
selects **M08-07 cumulative documentation**, not M09. Only after that closure is
accepted does M09-01 refresh the removal manifest and enter the planned deliberate
retirement work. Keep all reference source and independent optimization/profiling
provenance until that accepted transition. Append M08-07 documentation coverage
to this report after delivery without rewriting this review history.

## M08-07 cumulative documentation handoff — 2026-10-21

The lead's next-step instruction accepts M08-06 under the continuation convention;
backend/scenario detail remains unspecified. [Documentation coverage](documentation.md)
records nine new and twenty-two updated maintained pages (31 affected), actual
source/caller mappings, retained M01–M07 foundations and checks. Public standalone
context/baker/assembly/batch/writer, generic DB IO, model/prefab tutorial, full CLI
and architecture guidance are authored; the stale bake index and current DB
save-return/directory/hardlink instructions are corrected. Original review history
and all production coexistence references remain intact.

Static checks covered local targets/anchors/fences/includes/JSON, all parser flags,
API/example/source ordering, source-backed contracts and whitespace. No compile,
engine/native probe/test/package execution or source/CMake behavior change occurred.
The review's M08-07 authoring gap is addressed; documentation delivery awaits lead
acceptance. Its successor is M09-01 actual inventory/manifest, not automatic source
deletion. Native/evidence scope remains P01–P11 and the agreed acceptance policy.
