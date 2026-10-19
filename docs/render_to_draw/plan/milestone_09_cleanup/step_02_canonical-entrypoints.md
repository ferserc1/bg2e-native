# M09-02 — Draw-only application defaults and surviving products

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M09-01.

## Scope and deliverable

- `lib/include/bg2e/app/Application.hpp`
- `lib/include/bg2e/app/MainLoop.hpp`
- `lib/include/bg2e/app/OffscreenApplication.hpp`
- `lib/src/bg2e/app/detail/RenderGraphicsExecution.cpp`
- `apps/draw/model_edit/`
- `apps/draw/bg2e_composer/`
- `apps/draw/lightmap_generator/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/app/Application.hpp](../../../../lib/include/bg2e/app/Application.hpp), [lib/include/bg2e/app/MainLoop.hpp](../../../../lib/include/bg2e/app/MainLoop.hpp), [lib/include/bg2e/app/OffscreenApplication.hpp](../../../../lib/include/bg2e/app/OffscreenApplication.hpp), [lib/src/bg2e/app/detail/RenderGraphicsExecution.cpp](../../../../lib/src/bg2e/app/detail/RenderGraphicsExecution.cpp).

## Implementation work

1. Make ordinary MainLoop/Application/OffscreenApplication entry points select draw without requiring an experimental opt-in. Preserve explicit Vulkan/Metal backend configuration.
2. Remove production delegate/execution initialization branches and ensure public runtime Renderer switching/settings binding remains intact.
3. Remove obsolete app targets/source from supported builds; keep the requested apps/draw products as canonical source locations unless a deliberate relocation is recorded and all paths revised.
4. Finalize executable/app identifiers, preference/config identities and installed resource names. No legacy API forwarding is added to keep old applications running.

## M05 offscreen execution migration

The current offscreen application has two explicit init paths: the original
production-render delegate path and the experimental draw delegate path. M05
keeps ProductionRender as the default; it has not promoted draw. M09-02 must
make supported `OffscreenApplication` entry points select draw, preserve explicit
backend settings and migrate the new scene CLI to the canonical draw delegate,
then remove the old execution branch after M09-01's accepted manifest. Preserve
headless no-window/no-UI behavior, exact completed-frame callbacks and public
output contract while changing the default; no permanent experimental opt-in or
legacy compatibility shim remains in the M09 final product.

## Verification and exit conditions

- All supported windowed/headless/new application targets start through draw defaults and explicit backend override still works.
- No production Engine is linked/created by supported app entry points, including failure/cleanup paths.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M09-03/M09-05 update callers/build rules to final names. M10 must describe draw as production and remove obsolete opt-in guidance.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
