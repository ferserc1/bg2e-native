# M01-15 — Build and adapt examples/draw/deferred_renderer

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M01-01–M01-14.

## Scope and deliverable

- `examples/17_deferred_renderer/src/main.cpp`
- `examples/draw/README.md`
- `examples/CMakeLists.txt`
- `examples/draw/deferred_renderer/src/main.cpp`
- `examples/draw/deferred_renderer/CMakeLists.txt`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [examples/17_deferred_renderer/src/main.cpp](../../../../examples/17_deferred_renderer/src/main.cpp), [examples/draw/README.md](../../../../examples/draw/README.md), [examples/CMakeLists.txt](../../../../examples/CMakeLists.txt).

## Implementation work

1. Adapt the reference scene: OBJ two-submesh models with textures, opaque/transparent variants, floor, nested transforms, rotating camera/lights and HDR environment through draw Drawable and the new non-template delegate.
2. Retain supported material/sky/debug controls and add reflected settings, default scale/optional FSR selection and query soft-shadow controls. Remove only out-of-milestone AO/GI/reflection/denoise/manipulation/bake/offscreen controls.
3. Use MainLoop EngineConfig selection and paired pipeline shader libraries. Register a unique executable target/bundle and assets; do not implement another application loop.
4. Include controllable cutout/no-RT fixtures and a safe same-type renderer replacement action. Keep an explicit reference checklist of removed/deferred capabilities so M03 can restore effect controls.

## Verification and exit conditions

- The requested example runs on both available backends, resizes/pauses/resumes correctly and renders without RT/FSR.
- Clone/edit independence, material channels, query cutouts, settings-file round-trip and external upscaler injection have concrete evidence.
- No manipulation/app/bake/headless feature is required to run the example.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M02/M04/M05 derive their products from this scene. M03 updates this existing target rather than create a new example.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
