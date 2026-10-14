# M09 — Remove render and promote draw to production

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md)

**Status:** planned, not implemented. **Entry prerequisites:** All M01–M08 working products accepted with code-versus-plan reviews; actual removal manifest and unresolved parity checks reviewed first.

## Working product

The engine exposes draw as its only production rendering framework. The render namespace, legacy scene Drawables/bindings/visitors, production Engine overloads, obsolete application/example entry points and temporary compatibility support are removed. New applications remain the supported products; no old application API/binary support is retained.

## Scope exclusions

- No permanent render compatibility namespace, forwarding headers, native Drawable callbacks or old Engine overloads.
- No removal of Vulkan/Metal primitives, native SDK adapters or necessary GPU backend deployment merely because the production wrapper is deleted.
- No accidental deletion of independent CPU IO/scene behavior, authored assets or compatible stored scene/model data.

## Ordered steps

| Step | Deliverable |
|---|---|
| [M09-01](step_01_retirement-manifest.md) | Actual dependency inventory and deletion manifest |
| [M09-02](step_02_canonical-entrypoints.md) | Draw-only application defaults and surviving products |
| [M09-03](step_03_shared-api-cleanup.md) | Remove legacy shared overloads, scene resources and bindings |
| [M09-04](step_04_native-render-deletion.md) | Delete render implementation and confine native graphics to GPU |
| [M09-05](step_05_targets-tests-packages.md) | Retire legacy shaders/examples and repair tests/distribution |
| [M09-06](step_06_final-product-validation.md) | Draw production validation and zero legacy dependency checks |
| [M09-07](step_07_review.md) | Review M09 final production API and revise documentation plan |

## Milestone exit gate

- Supported source/build/runtime paths contain no dependency on bg2e::render or legacy scene::vk; draw is the default graphics execution path.
- All requested examples and new applications still run using final public names/defaults and installed resources.
- Native Vulkan implementation is confined to gpu, including remaining UI/SDK/platform graphics integrations; explicit binding indices/backend casts remain valid at named seams.
- Legacy app/example/test/build/script entries are removed or migrated, with no dead packaging paths or type shims.
- No MSAA/forward implementation remains in the new high-level renderer and no previous application compatibility is promised.
- M09 review inventories the actual final API/products and updates all final doc tasks.

Check Vulkan and Metal where the capability exists, plus a no-RT configuration and SDK-unavailable fallback. Product gates require actual evidence; static review or an unexecuted checklist cannot certify a working product. Build authorization follows the repository policy. Preserve production regression references through M08.

## Required code-versus-plan review

The last step is part of this milestone. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The final step lists milestone-specific comparisons. Do not rewrite completed history to conceal a design change.
