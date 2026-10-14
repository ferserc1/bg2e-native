# M03-09 — Integrate effects and verify existing examples only

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M03-04–M03-08.

## Scope and deliverable

- `lib/include/bg2e/draw/RendererDeferred.hpp`
- `examples/draw/deferred_renderer/src/main.cpp`
- `examples/draw/manipulation/src/main.cpp`
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

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M04/M05 products reuse this full scene renderer and its actual effect input/reset/convergence contracts.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
