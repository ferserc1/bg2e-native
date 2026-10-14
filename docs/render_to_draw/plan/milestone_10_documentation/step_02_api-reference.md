# M10-02 — Final API, boundaries, capabilities and extension reference

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M10-01.

## Scope and deliverable

- `doc/api/draw/`
- `doc/api/gpu/`
- `doc/api/reflection/`
- `doc/api/scene/`
- `doc/api/bake/`
- `doc/api/utils/`
- `doc/api/manipulation/`
- `doc/api/ui/`
- `doc/api/db/`
- `doc/api/app/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [doc/api/draw/](../../../../doc/api/draw), [doc/api/gpu/](../../../../doc/api/gpu), [doc/api/reflection/](../../../../doc/api/reflection), [doc/api/ui/](../../../../doc/api/ui), [doc/api/db/](../../../../doc/api/db), [doc/api/app/](../../../../doc/api/app).

## Implementation work

1. Update/create final public API indexes, reference/catalogs and lifecycle/ownership docs for draw, pipeline/environment, bake, utils tools and shared scene/manipulation/UI/app/DB paths.
2. Document ordinary accessor configuration plus reflected UI catalogs/stateless RenderSettingsPreferences, serialization schema/identifiers/precedence and runtime target rebinding.
3. Explain primitive capabilities versus runtime SDK/effect support, explicit Vulkan/Metal bindings and RT backend differences without introducing a fictitious shader abstraction.
4. Document public compatible object/shader injection, external renderer/upscaler hooks, deep CPU/GPU clones, safe waitIdle edits and lack of default instancing/MSAA/forward paths.

## Verification and exit conditions

- Declarations/signatures/type names/examples match final headers and registered property metadata.
- Public advanced objects are discoverable, while native backend details are correctly located in GPU and never taught as legacy high-level wrappers.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

Tutorial/application documentation uses this canonical reference; do not invent an API to simplify an explanation.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
