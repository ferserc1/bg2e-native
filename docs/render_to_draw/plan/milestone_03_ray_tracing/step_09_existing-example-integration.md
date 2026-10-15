# M03-09 — Integrate effects and verify existing examples only

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M03-04–M03-08.

## Scope and deliverable

- `lib/include/bg2e/draw/RendererDeferred.hpp`
- `examples/draw/10_deferred_renderer/src/main.cpp`
- `examples/draw/11_manipulation/src/main.cpp`
- `lib/include/bg2e/draw/RenderSettingsPreferences.hpp`
- `lib/include/bg2e/render/deferred/SMAAProcessor.hpp`
- `lib/include/bg2e/draw/pipeline/SMAAProcessor.hpp`
- `shaders/src/draw/smaa/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

## Implementation work

1. Attach optional AO/GI/reflection/temporal/denoise public objects via renderer/layer configuration and actual GPU capability checks.
2. Register effect settings dynamically, export/import stable options and expose implemented debug signals; reset on MaterialModifier/gizmo changes.
3. Restore effect controls deferred by M01 in its existing example and expose the same catalog in manipulation. No third example or duplicate application loop.
4. Verify all combinations relevant to production behavior, unsupported/disabled fallbacks and default versus FSR output; retain no-RT controls and clone/material regression fixtures.
5. Preserve the existing spatial-AA facility by reimplementing public SMAA processing/final-output components with paired shaders, lookup resources and required GPU formats. Make it an optional compatible pipeline component with its own accessor/reflected controls; the default stays simple resolution reduction. This is separate from explicitly excluded MSAA and adds no new example target.

## Verification and exit conditions

- Both examples exercise AO/GI switching, reflections, shadow controls, temporal/denoise, resize/pause/material edits and settings import on available backends.
- Primitive/effect unavailable states produce accurate UI and usable rendering, without forced FSR/full-RT capability coupling.
- The public optional SMAA component operates on no-RT raster output and can be selected/injected without hard-coding FSR as the only final processor.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M04/M05 products reuse this full scene renderer and its actual effect input/reset/convergence contracts.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-14 reflected effect UI

Use [GraphicsSettingsWindow](../milestone_01_deferred_renderer/settings-ui.md) and real catalog extensions for implemented effect settings. New targets appear through per-frame descriptor enumeration and safe queued application; do not recreate closed effect controls or a copied configuration model. Preserve explicit availability/status and catalog generations on resource replacement.

## M01-15 actual example

Extend target draw_deferred_renderer at examples/draw/10_deferred_renderer and the eventual manipulation product; do not create a new advanced-RT target. [working-example.md](../milestone_01_deferred_renderer/working-example.md) and the example README list retained/deferred reference capabilities. Current FSR is camera-only, query occluders exclude transparency, and debug modes show the last processed layer. Restore implemented effects through real catalog extensions.
