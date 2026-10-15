# M09-01 — Actual dependency inventory and deletion manifest

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** Accepted M08 review and all product evidence.

## Scope and deliverable

- `lib/include/bg2e/render/`
- `lib/src/bg2e/render/`
- `lib/include/bg2e/scene/vk/`
- `apps/`
- `examples/`
- `tests/`
- `cmake/`
- `scripts/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/](../../../../lib/include/bg2e/render), [lib/src/bg2e/render/](../../../../lib/src/bg2e/render), [lib/include/bg2e/scene/vk/](../../../../lib/include/bg2e/scene/vk), [apps/](../../../../apps), [examples/](../../../../examples), [tests/](../../../../tests), [cmake/](../../../../cmake), [scripts/](../../../../scripts).

## Implementation work

1. Scan includes, qualified types, aliases/macros, umbrella headers, CMake targets, tests, standalone templates, package/install/resource paths and indirect callers outside render.
2. Build a per-file remove/migrate/keep manifest distinguishing obsolete application APIs from neutral CPU functionality and still-needed GPU backends/SDK assets.
3. Cross-check every production feature against an accepted new product/API. Close required gaps before deleting its reference implementation; retain historical design documents as evidence, not maintained usage instructions.
4. Record the final public namespace/type/accessor/target naming choices and data-format compatibility expectations separately from deliberately unsupported old applications.

## Verification and exit conditions

- Every remaining render/scene::vk/legacy Drawable reference has an assigned replacement or deletion, including build/scripts/tests.
- The manifest does not schedule deletion of author assets, GPU Vulkan support or graphics-free scene/file functions.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M09-02–M09-06 execute this manifest; M10 uses its source-to-document mapping as the authority rather than the initial plan names.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-16 reviewed baseline

Include actual dual DrawableComponent accessors/mode tags, production scene visitors/scene::vk, ComponentFactoryRegistry/DB overloads, strong production versus weak draw TextureCache maps, legacy TextureWidgets native public members, UserInterfaceDelegate production init, application execution branches, manual tests/gpu/tests/draw probes, templates and paired shader deployment. A matching filename does not prove a replacement is implemented or accepted. See [M01 review](../milestone_01_deferred_renderer/review.md).
