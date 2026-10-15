# M01-07 — Graphics-free serialization and simple engine-aware load paths

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M01-06.

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

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M06-02/M07-02/M08-02 complete resource IO on these two levels. Record actual factory signatures/custom-component fallback requirements for those steps.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-06 resource handoff

M01-06 supplies `draw::Drawable` (standard geo::MeshPNUUT), live material access and typed DrawableComponent construction/setters/drawDrawable/executionMode. Implement draw-aware load/save and component serialization explicitly; the legacy component serializer rejected draw data at M01-06 to prevent silent omission. GPU-only material textures can be cloned by exact transfer but require explicit image readback for export; adopted images without a source cannot be reloaded from an invented path. The existing image loader now owns SDR/HDR pixels consistently with base::Image. See [drawable-resources.md](drawable-resources.md).

## Implemented M01-07 handoff

Actual API, custom component requirements, resource ownership and lead checks are recorded in [db-levels.md](db-levels.md). Representation-only IO is explicitly named readSceneDocument/writeSceneDocument/saveSceneReferences. Full draw resource export remains scheduled in M06/M07; it must not be confused with reference serialization.
