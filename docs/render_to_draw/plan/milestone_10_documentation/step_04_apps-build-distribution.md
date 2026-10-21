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

M05's current interim product is `draw_offscreen_application`, with a typed draw
`OffscreenApplication` overload, explicit backend/settings/resolution/frame
options, final R8G8B8A8_UNORM output, exact linear camera accumulation and a
verified no-clobber PNG. Its readback is synchronous and full-subresource; no
HDR/BGRA/sRGB conversion is supplied by `readPixelsRGBA8`. This is not M06/M07
complete scene/resource export or M08 batch output. At M10 audit the final
canonical executable name and docs after M09, settings precedence, exact requested
sample count, output dimensions/format, failure/overwrite behavior and bundled
resources against the final source; don't assume the interim target name survives.


## M04-07 reviewed implementation requirement

Audit both draw and bake native library build/install/bundle refresh paths. Include the missing-metallib regression and check the final installed source/target names after M09. Saved sampler metadata for a GPU-only image is not complete model/batch output; trace real file materialization to the accepted M06/M08 implementations.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).

M06's source-level package review confirms `draw_model_edit` uses `bundle_app_sdl`
and the recursive distribution collector installs registered apps, runtime
dependencies, shaders and assets. It records no native package launch or feature
parity execution. Use the [M06 product validation record](../milestone_06_model_edit/product-validation.md)
as an outstanding evidence list, then replace interim target/resource names only
after checking the final M09 source and native installed layout.

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
