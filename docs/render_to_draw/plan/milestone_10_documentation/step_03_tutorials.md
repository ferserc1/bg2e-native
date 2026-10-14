# M10-03 — Runnable tutorials for delivered examples and customization

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M10-01/M10-02.

## Scope and deliverable

- `doc/tutorials/draw/`
- `doc/tutorials/gpu/`
- `doc/deferred_render_model.md`
- `doc/delegate_system.md`
- `doc/lightmap_baking.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [doc/tutorials/draw/](../../../../doc/tutorials/draw), [doc/tutorials/gpu/](../../../../doc/tutorials/gpu), [doc/deferred_render_model.md](../../../../doc/deferred_render_model.md), [doc/delegate_system.md](../../../../doc/delegate_system.md), [doc/lightmap_baking.md](../../../../doc/lightmap_baking.md).

## Implementation work

1. Document each delivered example: basic deferred/soft shadows/FSR default scale, manipulation/MaterialModifier, complete effects/histories, integrated lightmaps and accumulated offscreen output.
2. Provide focused runtime renderer replacement, public pass/subclass/shader injection and application-owned external upscaler demonstrations using actual final APIs.
3. Explain no-RT fallback, query versus advanced RT kernels, matched shader folders/binding indices, settings file round-trip and headless parameter precedence.
4. Update or retire traditional forward/render tutorials and architectural explanations that now reference deleted classes. Retain valid primitive GPU examples with correct current behavior.

## Verification and exit conditions

- Tutorial snippets correspond to final example call sites; all referenced targets/assets/shader libraries exist.
- Steps clarify actual accumulated-frame and UV-map semantics, clone independence and optional capability limits without claiming unsupported implementations.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M10-04 aligns app/CLI instructions and M10-05 validates linked snippets/navigation across the whole documentation set.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
