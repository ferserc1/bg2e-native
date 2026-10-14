# M10 — Final documentation for the production engine

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md)

**Status:** planned, not implemented. **Entry prerequisites:** Accepted M09 final API/product/source-to-document mapping; read actual final code, not only initial plan names.

## Working product

The maintained documentation under `doc/` accurately describes draw as production, gpu native primitives/capabilities, public draw::pipeline/environment extension points, bake/utils/manipulation, two-level DB IO, reflected settings and all supported examples/applications. Old rendering usage/experimental-opt-in guidance is removed.

## Scope exclusions

- No new engine features, replacement renderer, ResourceManager, Metal native upscaler or external DLSS implementation.
- No old-application API compatibility documentation or working render namespace snippets.
- Historical analysis/planning under docs/ remains labeled history; it does not override current maintained API docs.

## Ordered steps

| Step | Deliverable |
|---|---|
| [M10-01](step_01_documentation-inventory.md) | Inventory every doc page against final source |
| [M10-02](step_02_api-reference.md) | Final API, boundaries, capabilities and extension reference |
| [M10-03](step_03_tutorials.md) | Runnable tutorials for delivered examples and customization |
| [M10-04](step_04_apps-build-distribution.md) | Application, CLI, build and final breaking-change docs |
| [M10-05](step_05_documentation-validation.md) | Links, snippets and complete maintained-doc consistency |
| [M10-06](step_06_final-review.md) | Final code/documentation review and close the implementation plan |

## Milestone exit gate

- Every maintained page under doc/ is reviewed for namespace/type/path/status/lifecycle/binding/schema accuracy.
- Public advanced components and compatible injection/subclass examples are documented, not presented as inaccessible internals.
- Settings export/import, runtime renderer/upscaler rebinding, deep GPU clone, safe rare edits and graphics-free versus complete IO are explicit.
- Tutorials/apps/build/package/offscreen/bake instructions point to final supported targets and resources.
- Broken links/stale code snippets/obsolete render or MSAA/forward promises are removed from maintained usage documentation.
- M10 final review compares actual code and all delivered products with the completed plan and lists only explicit out-of-scope follow-up work.

Check Vulkan and Metal where the capability exists, plus a no-RT configuration and SDK-unavailable fallback. Product gates require actual evidence; static review or an unexecuted checklist cannot certify a working product. Build authorization follows the repository policy. Preserve production regression references through M08.

## Required code-versus-plan review

The last step is part of this milestone. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The final step lists milestone-specific comparisons. Do not rewrite completed history to conceal a design change.
