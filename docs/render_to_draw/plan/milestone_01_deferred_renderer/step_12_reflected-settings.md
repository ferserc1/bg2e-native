# M01-12 — Stateless preferences facade, dynamic catalog and settings codec

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M01-05/M01-10/M01-11.

## Scope and deliverable

- `lib/include/bg2e/reflection/Property.hpp`
- `lib/include/bg2e/reflection/Builder.hpp`
- `lib/include/bg2e/reflection/TypeInfo.hpp`
- `lib/include/bg2e/draw/RenderSettingsPreferences.hpp`
- `lib/src/bg2e/draw/RenderSettingsPreferences.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/reflection/Property.hpp](../../../../lib/include/bg2e/reflection/Property.hpp), [lib/include/bg2e/reflection/Builder.hpp](../../../../lib/include/bg2e/reflection/Builder.hpp), [lib/include/bg2e/reflection/TypeInfo.hpp](../../../../lib/include/bg2e/reflection/TypeInfo.hpp).

## Implementation work

1. Bind reflection accessors to active renderer/layer/environment/upscaler objects for UI-facing properties. Keep ordinary object accessors and configuration as the sole values; the facade stores only targets/metadata/persistence plumbing.
2. Expose instance-specific enumeration with type/category/range/enum options and capability visibility. Use a catalog generation to invalidate UI handles after renderer/component changes; leave shared TypeRegistry metadata intact.
3. Implement persistence plus versioned JSON serialize/deserialize/export/import using reflected getters/setters, exact any types and stable property/component identifiers. Enumerated options are identities, not list indices.
4. Validate complete import before mutation and apply through object setters at a safe boundary. Specify unknown/unavailable property diagnostics and persisted-default versus explicit-file precedence without retaining a shadow value model.

## Verification and exit conditions

- Programmatic setter changes are immediately visible to UI/export; imported values and persistence change the same object and survive a restore round-trip.
- CPU-only codec checks cover malformed types/enums/ranges and unsupported properties; renderer replacement clears stale property references.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M01-13/M01-14 complete upscaler/UI registration. M03/M04 add properties; M05 uses the same settings codec with no UI.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
