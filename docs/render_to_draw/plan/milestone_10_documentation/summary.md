# M10 — Final audit of cumulative documentation

[Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Review protocol](../review-protocol.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Entry prerequisites:** Accepted M09-08 documentation closure, final source/product baseline and all M01–M09 documentation handoffs.

M02's accepted review is the source-to-plan record for manipulation identity,
synchronous picking/viewpoint readback, draw overlay and preview lifetime,
partial CPU/live material edits, clone limits and unverified product behavior.
M10 reconciles those contracts with final APIs and maintained docs; it does not
upgrade the M02 continuation assumption into backend-specific runtime evidence.

## Working product

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
