# M01-07 — Graphics-free serialization and simple engine-aware load paths

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M01-06.

## Scope and deliverable

- `lib/include/bg2e/db/scene.hpp`
- `lib/include/bg2e/db/mesh_obj.hpp`
- `lib/include/bg2e/db/mesh_bg2.hpp`
- `lib/include/bg2e/scene/ComponentFactoryRegistry.hpp`
- `lib/include/bg2e/scene/Component.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/db/scene.hpp](../../../../lib/include/bg2e/db/scene.hpp), [lib/include/bg2e/db/mesh_obj.hpp](../../../../lib/include/bg2e/db/mesh_obj.hpp), [lib/include/bg2e/db/mesh_bg2.hpp](../../../../lib/include/bg2e/db/mesh_bg2.hpp), [lib/include/bg2e/scene/ComponentFactoryRegistry.hpp](../../../../lib/include/bg2e/scene/ComponentFactoryRegistry.hpp), [lib/include/bg2e/scene/Component.hpp](../../../../lib/include/bg2e/scene/Component.hpp).

## Implementation work

1. Separate representation/file serialization from engine-aware resource loading. Low-level scene/component operations preserve unresolved resource references without booting either engine.
2. Provide draw overloads of the minimal high-level mesh/scene load helpers and initialization hooks needed by the first example. Reuse pure CPU OBJ/.bg2 parsing rather than force production upload.
3. Keep production high-level overloads operational. Remove engine dependencies in schema-only functions; retain simple initialized-mode overloads where upload genuinely needs an engine.
4. Define high-level save/readback responsibilities for GPU-only images, but implement full resource export/import in M06/M07. Do not call a metadata-only write a complete save.

## Verification and exit conditions

- A schema/reference round-trip can run without Engine/SDL/device initialization.
- The initial example loads its mesh/textures using only the draw path; production load convenience functions still work.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M06-02/M07-02/M08-02 complete resource IO on these two levels. Record actual factory signatures/custom-component fallback requirements for those steps.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
