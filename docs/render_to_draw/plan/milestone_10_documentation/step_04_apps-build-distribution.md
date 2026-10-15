# M10-04 — Audit applications, CLI and distribution guidance

[Milestone summary](summary.md) · [Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Prerequisites:** M10-01–M10-03 and accepted M09-08.

## Scope and deliverable

Audit existing maintained `doc/` pages and M01–M09 documentation handoffs against final source, products and distributed coverage. Record findings and localized corrections for the final M10 review. This step does not postpone earlier documentation authoring to M10.

## Audit work

1. Audit existing apps/draw guides against final model editor/composer/lightmap generator executables and current UI/import/material/viewpoint/bake/async/safe-edit workflows.
2. Check offscreen/batch CLI settings/precedence, output paths/counts/formats/preflight/errors/overwrite behavior against implemented parsers and products.
3. Check build generators, SDK/resources/native Vulkan/Metal deployment, package/standalone/library-update guidance against current scripts and installed layout.
4. Check actual breaking changes/draw promotion and removed old APIs. Make localized corrections without promising compatibility shims or unsupported features; missing substantial application/distribution guidance must be repaired under its distributed owner.

## Verification and exit

Existing application and deployment instructions point to correct supported targets, resources and actual behavior.

Perform relevant static checks. Compilation/runtime validation belongs to the project lead unless explicitly requested. Deliver one step, update progress and wait for lead acceptance under the execution protocol. No new engine feature, build-system change or future API documentation is authorized by this audit.
