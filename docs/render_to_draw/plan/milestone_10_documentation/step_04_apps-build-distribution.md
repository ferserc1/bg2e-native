# M10-04 — Audit applications, CLI and distribution guidance

[Milestone summary](summary.md) · [Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Prerequisites:** M10-01–M10-03 and accepted M09-08.

## Scope and deliverable

Audit existing maintained `doc/` pages and M01–M09 documentation handoffs against final source, products and distributed coverage. Record findings and localized corrections for the final M10 review. This step does not postpone earlier documentation authoring to M10.

## Audit work

Use [M07 product/review evidence](../milestone_07_bg2e_composer/review.md) for
Composer scene/Chain/table controls, local/remote import queue and port conflict,
cooperative parser cancellation, joined shutdown and per-target bake previews/
temporary sources. Reconcile these with canonical post-M09 app/settings identities.
The lead-checked selection correction is not a recorded native package/HTTP matrix.

1. Audit existing apps/draw guides against final model editor/composer/lightmap generator executables and current UI/import/material/viewpoint/bake/async/safe-edit workflows.
2. Check offscreen/batch CLI settings/precedence, output paths/counts/formats/preflight/errors/overwrite behavior against implemented parsers and products.
3. Check build generators, SDK/resources/native Vulkan/Metal deployment, package/standalone/library-update guidance against current scripts and installed layout.
4. Check actual breaking changes/draw promotion and removed old APIs. Make localized corrections without promising compatibility shims or unsupported features; missing substantial application/distribution guidance must be repaired under its distributed owner.

## Verification and exit

Existing application and deployment instructions point to correct supported targets, resources and actual behavior.

Perform relevant static checks. Compilation/runtime validation belongs to the project lead unless explicitly requested. Deliver one step, update progress and wait for lead acceptance under the execution protocol. No new engine feature, build-system change or future API documentation is authorized by this audit.


## M05 offscreen CLI source-of-truth

The final canonical camera CLI target is `draw_offscreen_application` under
`examples/draw/13_offscreen_application`. Document the current
DrawOffscreenApplicationConfig/Delegate and OffscreenApplication contracts,
explicit backend/settings/resolution/frame options, final R8G8B8A8_UNORM output,
exact requested camera accumulation and no-clobber PNG behavior. Readback is
synchronous and full-subresource; readPixelsRGBA8 supplies no HDR/BGRA/sRGB
conversion. Audit settings precedence, requested sample count, dimensions,
failure/overwrite behavior and bundled resources against final source. Complete
scene/resource export and standalone bake batch output have separate contracts.


## M04-07 reviewed implementation requirement

Audit both draw and bake native library build/install/bundle refresh paths. Include the missing-metallib regression and check the final installed source/target names after M09. Saved sampler metadata for a GPU-only image is not complete model/batch output; trace real file materialization to the accepted M06/M08 implementations.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).

M06's source-level package review confirms `draw_model_edit` uses `bundle_app_sdl`
and the recursive distribution collector installs registered apps, runtime
dependencies, shaders and assets. It records no native package launch or feature
parity execution. Use the [M06 product validation record](../milestone_06_model_edit/product-validation.md)
as a source of historically unspecified native coverage; audit the final canonical
names in final-products.tsv and distinguish source layout from executed installation.

## Current engine preservation requirements — 2026-10-21

Audit current Composer Window → Performance, live charts without file creation,
Start/Stop Recording and close/reopen/shutdown behavior. Graphics Settings no longer
owns profiler controls. Preserve final application-specific integration rather than
inventing Model Edit/headless UI parity. Deployment retains v3/v5 native shader and
optional timing backend support; include bool save failures and copy fallback in
output documentation without claiming whole-batch atomicity from generic DB IO.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-06 final standalone product/deployment audit

Compare final surviving lightmap app parser/target with its M08 typed bake schema,
CLI-last overrides, input/copy/UV2 rules, signal/exit and complete-manifest/native
HDR contract. Reconcile any intentional executable rename with app IDs, CMake,
package discovery and scripts/templates/main.cpp; binary naming does not implicitly
migrate data profiles. Check actual installed/relocated paired draw/bake stages
and full model sidecars, retaining artifact-inspector and native reload evidence
limits. M08 source registration is not measured package/backend success.

## M09-05 implemented distribution contracts

Use [the product delivery](../milestone_09_cleanup/products-distribution.md),
scripts/PACKAGING.md and current CMake/scripts as source. The only registered
apps are draw_model_edit, draw_bg2e_composer and draw_lightmap_generator; IDs,
preferences and Composer server behavior are unchanged. Retired examples/GLSL/
flat compiler frontends must not appear as available instructions. Native GPU/
ShaderLib custom-example helpers remain supported. Document paired draw/bake
aggregate targets, unique Metal intermediates, incremental copies, RuntimeShaders
and authoritative Distribution install resources. Standalone creation now uses
the deferred template; updates replace headers/library/draw/bake and helper with
custom resource preservation. Describe actual shell/PowerShell/Xcode paths and
separate file-presence/static checks from lead-reported launch/package evidence.

## M09-06 final product/verification boundary

Use final-products.tsv and validation-matrix.tsv in M09, with
[final-product-validation.md](../milestone_09_cleanup/final-product-validation.md).
The source catalog names 33 registered targets and hashes their C++ inputs.
Actual native installed/relocated outputs, exported symbols, generated standalone
projects and decoded artifacts remain lead-owned scenario evidence. Literal
standalone template substitution and source resource rules establish no launch
result. Preserve this distinction while auditing the accepted M09-08 guidance.

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

Audit draw_model_edit, draw_bg2e_composer and draw_lightmap_generator under apps/draw; draw_offscreen_application remains the camera render example. Keep configuration identities and import server protocols. Verify paired draw/bake shader trees, optional FFX/SMAA, standalone deferred default, updater scope and native SDK/runtime copies against current scripts. Reused output directories do not imply purging stale flat artifacts. Creator/package/launch behavior is source-reviewed unless lead execution evidence specifies otherwise.

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

## Source correction delivered — 2026-10-22

The user subsequently authorized source repair. Both findings are corrected in
[source-corrections.md](source-corrections.md): GPU 11/13 serialize shared UBO access,
and all four standalone helpers check the canonical paired shader paths. Earlier
finding/handoff paragraphs and stored audit hashes above are historical snapshots,
not current open issues. Native validation and final lead acceptance remain pending.
