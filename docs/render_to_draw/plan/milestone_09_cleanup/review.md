# M09-07 — Final production API review

[Progress](../progress.md) · [Documentation successor](step_08_documentation.md)

## Revision and product status

Reviewed 2026-10-21 at HEAD `1889bdb12ade85d820eaab4a7365760fa9c34f50`
plus the inherited uncommitted implementation. M09-06 and its three corrections
are accepted by the project lead's instruction to continue, under the execution
protocol. Backend/scenario coverage beyond the reported macOS arm64 compiler and
linker failures is unspecified. This review awaits acceptance; M09 is not closed.

Production is `bg2e::draw`. `bg2e::gpu` supplies backend primitives; retired
`bg2e::render` and old application binaries have no compatibility contract.
The final source audit covers 1,210 files with source-set SHA-256
`d7a909b07bdf8ecbb0b7c61fdb83a2866ec16864fbda07d637ed7e467bce7bb7`.
No engine/product/CMake/script/shader/profiling implementation changes are part
of M09-07. The step-entry preservation snapshot covers 1,327 files.

## Planned and actual contracts

All header paths below are relative to `lib/include/bg2e/`.

| Contract | Final implementation and classification | Documentation owner / audit |
|---|---|---|
| Production renderer | `draw/Renderer.hpp`, `RendererDeferred.hpp`, `DefaultRenderLoopDelegate.hpp`; exclusive renderer ownership with retained shared injected components. Equivalent lifecycle refinement. | M09-08; M10-02/03 |
| Optional components | RendererConfiguration selects layers, environment, shadows, upscaler, resolution, AO/GI/reflections, temporal/denoise/final processing and overlays. Null policy is slot-specific: AO disabled, GI/reflections defaults enabled, upscaler bilinear; document declarations rather than a universal null rule. | M09-08; M10-02 |
| Lifecycle | Initialize, scene/update, prepareFrame before acquisition, recording and cleanup; resize/history invalidation and optional linear/final recording hooks. Borrowed frame bindings must not escape their recording lifetime. | M09-08; M10-02/03 |
| Upscaling | `draw/Upscaler.hpp`, `FsrUpscaler.hpp`: portable requirements, availability/status, preparation, recording/fallback, reset/cleanup and reflected settings. Optional SDK unavailability is an advertised fallback. Native Metal upscalers/DLSS are deliberately excluded. | M09-08; M10-02/03 |
| Settings | `draw/SettingsCatalog.hpp`, `SettingsCodec.hpp`, `RenderSettingsPreferences.hpp`: live instances and optional shared owners, generation-safe handles/queued changes, strict typed import and explicit unknown-setting policy. Integer persistence uses decimal strings to avoid float JSON precision loss. No duplicate persistent settings model. | M09-08; M10-02/05 |
| Scene and clone | `draw/Drawable.hpp`, `scene/DrawableComponent.hpp`, `ComponentFactoryRegistry.hpp`: canonical shared drawable access, retained serialized `Drawable` identity, deep mesh/attribute/material/texture clones, GPU resources when loaded. `material()` is a live loaded alias; `materialDefinition()` has distinct semantics. Factory selection has no execution mode flag. | M09-08; M10-02 |
| IO and manipulation | `db/scene.hpp`, draw resource IO and canonical manipulation APIs; complete save snapshots, archive/parent/rollback behavior retained. CPU parsing/root transfer remain distinct from GPU resource creation. Mesh/selection APIs are draw-only. | M09-08; M10-02/04 |
| Bake API | `bake/BakerContext.hpp`, `StandaloneBakerContext.hpp`, `StandaloneLightmapBaker.hpp`, `StandaloneBakeSceneAssembler.hpp`, `StandaloneBakeBatch.hpp`, `LightmapOutputWriter.hpp`. Implemented names replace tentative owners. One live target/context; borrowed device/command, global submission serial, retained pass owners, exclusive bake lifecycle and chart isolation. Stop/drain before teardown; never share mutable display slots. | M09-08; M10-02/03/04 |
| Native GUI boundary | Public GPU GUI/window APIs use engine primitives; UI owns ImGui context/platform pairing. Private adapters bridge to GPU without exposing ImGui in public headers. Equivalent backend isolation. | M09-08; M10-02 |
| Product promotion | Three apps under `apps/draw`, 13 draw examples, 13 GPU examples and four test targets. See final-products.tsv for exact target/source paths. Old registrations and sources are removed. | M09-08; M10-01/03/04 |
| Shaders/distribution | Paired draw/bake source trees, collision-free native stage outputs, preserved native SDK/runtime and optional FFX/SMAA paths. Standalone default is deferred draw; updater refreshes engine headers/library/shaders/helper. Reused output folders are not automatically purged. | M09-08; M10-04/05 |
| Optimization/profiling | R01–R14 remain independent preservation requirements: revisions, persistent arrays, completion/acceleration reuse, pools/leases, temporal and transparent fallbacks, typed synchronization, instrumentation and UI. Retained source/probes are not measured performance evidence. | M09-08; M10-01/02/05/06 |

Private execution helpers and UI adapters retain necessary separation between
application lifecycle, native command/encoder ownership and context-specific UI
state. They do not create another public renderer API. Backend-specific bindings
and shaders remain explicit: this review introduces no shader translator or
higher-level GPU dependency. Complete-object settings pointers support multiple
inheritance; replacing them with an unadjusted base pointer would change behavior.

## Retirement reconciliation and completed scope

[retirement-reconciliation.tsv](retirement-reconciliation.tsv) resolves every one
of the 1,620 original manifest paths against current files and the chronological
entrypoint/shared/native/product/correction ledgers. All original `remove` paths
are absent; every changed path has a ledger owner. The original manifest and its
hashes remain unchanged. Across final delta ledgers there are 593 distinct paths,
440 absent and 153 surviving; the earlier four-ledger count of 592 predates the
additional correction owner. A planned migrate/keep file's removal is resolved
by its actual delta owner and replacement/preservation entry, not by claiming
that a textual match is a symbol proof.

The final header catalog contains 317 headers; eleven umbrella entry roots reach
255 headers. Explicit/backend headers outside those umbrellas remain supported.
All 33 registrations are cataloged in [final-products.tsv](final-products.tsv).
Manual fixtures/profiling probes survive; they are not an executed test suite.
The old dual execution API and native render wrappers are removed deliberately.
No missing runtime feature was discovered by this review. Cumulative maintained
documentation remains incomplete until M09-08; native/performance coverage is a
verification limitation, not permission to remove preserved implementations.

## Verification and correction attribution

| Evidence | Result and limit |
|---|---|
| Agent final source audit | Re-run audit_final_sources.py: 1,210 files, zero missing bg2e includes/retired source matches, 33 products and literal standalone closure. Lexical source evidence only. |
| Agent native audit | Re-run audit_native_sources.py: GPU dependency boundary retained, sole VMA implementation in gpu/vk/Device.cpp; corrected hashes incorporated. |
| Agent product audit | Re-run audit_product_sources.py: registrations, retained probes and 292 collision-free paired outputs / 290 include edges. No shader compilation. |
| Manifest review | All 1,620 rows reconciled; no retained planned removals and no changed path without a delta owner. |
| Page inventory | 232 maintained Markdown pages hashed/classified in documentation-handoff.tsv. Classification is authoring input, not completed page review. |
| Lead failures/corrections | Restored neutral countSceneNodes traversal; enabled IMGUI_IMPL_METAL_CPP locally in relocated Metal source; restored the sole global VMA implementation with platform diagnostics/logging in existing Device.cpp. See validation-corrections.tsv and final-product-validation.md for exact hashes/history. |
| Lead acceptance | Continuation accepts M09-06 including all three corrections. No further build/runtime/backend details supplied. Prior pending wording remains historical. |
| Agent execution | No configure, build, native test, package/creator execution, output round-trip, binary export inspection or performance measurement. |

Source-preservation hashes verify that review work does not reformat inherited
C/C++/shader code. No new source files or cosmetic code edits are introduced.
The native correction blocks retain the repository license, global macro scope,
namespace layout and braced control flow. This review does not certify a complete
repository-wide cosmetic reformat or parse C++ symbols from lexical catalogs.

## Pending-plan changes and documentation mapping

[documentation-handoff.tsv](documentation-handoff.tsv) inventories every current
maintained Markdown page with its hash, family, disposition and owners. M09-08
must record final dispositions, new destinations, new pages and retired incoming
links in documentation.md; token classification alone does not fulfill that task.
API pages must follow actual declarations, tutorial pages actual catalog targets,
app pages actual canonical applications, and architecture/build pages the runtime
and script contracts above. Foundation/math/json/reflection/input pages must be
reviewed and retained where applicable, not deleted with retired render pages.

| Edited pending file | Concrete responsibility |
|---|---|
| M09 step_08_documentation.md | Complete cumulative authoring, page dispositions and source/product mapping before milestone closure. |
| M10 summary.md | Audit the delivered complete documentation, with R01–R14 and product coverage. |
| M10 step_01_documentation-inventory.md | Reconcile every page and new/retired destinations against the M09-08 handoff. |
| M10 step_02_api-reference.md | Final renderer/settings/upscaler/clone/IO/bake/native contracts and implemented names. |
| M10 step_03_tutorials.md | Current draw/GPU targets, no-RT/upscaler/shader tutorials and standalone bake lifetime. |
| M10 step_04_apps-build-distribution.md | Canonical app/offscreen identities, standalone/package helpers and paired resources. |
| M10 step_05_documentation-validation.md | Links/snippets/diagrams, exact capacities/bindings and accurate verification claims. |
| M10 step_06_final-review.md | Complete requirements/page/product reconciliation and honest final evidence. |

Root index, architecture decisions, requirements map and source anchors now point
to this final baseline while retaining historical planning intent. Historical
retired source links in the original anchor table are plain historical paths.

## Remaining gates

Deliver M09-07 for lead acceptance, then implement M09-08 only. M09 closes after
its cumulative documentation delivery is accepted; M10 follows as the final
audit. There is no new architecture decision requiring user input. Do not infer
native/backend/performance results or repeat compilation based on this review.

## M09-08 cumulative documentation handoff — 2026-10-21

M09-07 accepted by project lead continuation. M09-08 updates the maintained
documentation to final production draw/bake contracts; see
[documentation.md](documentation.md), per-page dispositions and the static audit.
There are 229 final maintained pages and a complete 33-product guide catalog.
All 1,327 step-entry implementation/profiling files are unchanged. No build or
native/performance result is claimed. M09-08 awaits acceptance; its successor is
M10-01, the final inventory audit. Earlier review sections retain delivery history.
