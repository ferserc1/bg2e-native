# M09 — Remove render and promote draw to production

[Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Review protocol](../review-protocol.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Entry prerequisites:** All M01–M08 working products accepted with code-versus-plan reviews; actual removal manifest and unresolved parity checks reviewed first.

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
| [M09-08](step_08_documentation.md) | Update maintained documentation cumulatively through M09 |

## Milestone exit gate

- The final [documentation step](step_08_documentation.md) updates maintained `doc/` through this milestone and is accepted before the next milestone; no future implementation is documented as available.

- Supported source/build/runtime paths contain no dependency on bg2e::render or legacy scene::vk; draw is the default graphics execution path.
- All requested examples and new applications still run using final public names/defaults and installed resources.
- Native Vulkan implementation is confined to gpu, including remaining UI/SDK/platform graphics integrations; explicit binding indices/backend casts remain valid at named seams.
- Legacy app/example/test/build/script entries are removed or migrated, with no dead packaging paths or type shims.
- No MSAA/forward implementation remains in the new high-level renderer and no previous application compatibility is promised.
- M09 review inventories the actual final API/products and updates all final doc tasks.

Provide the project lead with applicable Vulkan/Metal, no-RT and SDK-unavailable checks. The lead compiles/tests after each step; the agent performs static inspection and does not run builds/runtime checks unless explicitly instructed. Their instruction to continue accepts the preceding delivered step under the [execution protocol](../execution-protocol.md), without repeated confirmation or mandatory test artifacts. Record who checked what and leave unspecified coverage unspecified. Preserve production regression references through M08.

## Required code-versus-plan review

The technical review is the penultimate step; the cumulative documentation update is the final required step. Create `review.md` only after execution with actual revisions/contracts/results. Compare the code against every pending milestone and update affected summaries and step files, including dependencies, paths and validation. The review step lists milestone-specific comparisons; the documentation step closes the milestone under the cumulative documentation policy. Do not rewrite completed history to conceal a design change.

## M01 review handoff

Retire actual coexistence artifacts only after accepted application/CLI replacements. Inventory dual scene/DB/cache/UI paths, public legacy native members, execution branches, probes, SDK bridge, shader deployment and templates. Preserve data readability independently of deliberately removed application API compatibility. See [M01 review](../milestone_01_deferred_renderer/review.md) for source evidence, classifications and verification limits. Milestone order and product gate remain unchanged.

## Documentation closure order

The technical review now hands off to [M09-08](step_08_documentation.md), not directly to the next milestone. Earlier next-milestone handoff statements are historical and superseded by this order. See [documentation policy](../documentation-policy.md).
