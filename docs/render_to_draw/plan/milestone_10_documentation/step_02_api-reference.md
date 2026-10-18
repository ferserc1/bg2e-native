# M10-02 — Audit final API reference and extension contracts

[Milestone summary](summary.md) · [Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Prerequisites:** M10-01.

## Scope and deliverable

Audit existing maintained `doc/` pages and M01–M09 documentation handoffs against final source, products and distributed coverage. Record findings and localized corrections for the final M10 review. This step does not postpone earlier documentation authoring to M10.

## Audit work

1. Audit existing API indexes/reference and lifecycle/ownership against final headers for draw/pipeline/environment, GPU/reflection/scene/bake/utils/manipulation/UI/DB/app.
2. Check accessor and reflected settings catalogs, identifiers/schema/precedence, import/export and runtime rebinding; verify clone ownership, safe rare edits, graphics-free versus complete IO and public component injection.
3. Check explicit backend bindings/native boundaries, primitive versus SDK/runtime capabilities and genuine Vulkan/Metal differences. Confirm no unsupported ResourceManager, Metal upscaler, DLSS, forward/MSAA or compatibility promise.
4. Correct localized inconsistencies in existing pages. Missing substantive reference belongs to its M01–M09 documentation owner; do not invent unimplemented APIs.

Include the [M02 review](../milestone_02_manipulation/review.md) in the source
map: distinguish transient selection/picking IDs from queue/TLAS identities,
document synchronous RGBA8/D32 operations and stopped-boundary requirements,
separate stored CPU material definitions from live attributes, and describe
preview/overlay/gizmo ownership and cleanup. Preserve the documented M02 evidence
limit: project-lead continuation is acceptance under protocol, with later
backend/scenario coverage unspecified.

## Verification and exit

Existing API documentation agrees with final declarations, metadata, capabilities and public extension points.

Perform relevant static checks. Compilation/runtime validation belongs to the project lead unless explicitly requested. Deliver one step, update progress and wait for lead acceptance under the execution protocol. No new engine feature, build-system change or future API documentation is authorized by this audit.

## M03-10 reviewed implementation requirement

Audit SurfaceInputs formats, snapshot capacity/lifetime, signal normalization/confidence, requested settings versus availability and example-owned transforms against final source. Distinguish bounded display histories from exact bake/offscreen accumulation and final SMAA from denoise; preserve retirement-era renames without retaining deleted APIs.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.


## M04-07 reviewed implementation requirement

Audit M04's actual contracts: one live integrated target/open frame, exact valid counts, AO/GI normalization and zero-reference policy, strict half-float raw-GI saturation rejection, nearest-seed dilation, RGB32F versus display-half limits, explicit indirectTextureMode, CPU UV preview and UI-session resource ownership. Standalone/batch/export APIs are future M08 implementations until their owning deliveries.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).
