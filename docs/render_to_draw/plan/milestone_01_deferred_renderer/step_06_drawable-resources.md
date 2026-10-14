# M01-06 — Draw Drawable, materials/textures and dual scene component

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M01-01/M01-03/M01-05.

## Scope and deliverable

- `lib/include/bg2e/scene/Drawable.hpp`
- `lib/src/bg2e/scene/Drawable.cpp`
- `lib/include/bg2e/scene/DrawableComponent.hpp`
- `lib/include/bg2e/draw/Drawable.hpp`
- `lib/include/bg2e/draw/Texture.hpp`
- `lib/include/bg2e/draw/MaterialBase.hpp`
- `lib/include/bg2e/utils/TextureCache.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/scene/Drawable.hpp](../../../../lib/include/bg2e/scene/Drawable.hpp), [lib/src/bg2e/scene/Drawable.cpp](../../../../lib/src/bg2e/scene/Drawable.cpp), [lib/include/bg2e/scene/DrawableComponent.hpp](../../../../lib/include/bg2e/scene/DrawableComponent.hpp), [lib/include/bg2e/utils/TextureCache.hpp](../../../../lib/include/bg2e/utils/TextureCache.hpp).

## Implementation work

1. Reimplement the current connector in draw using gpu mesh/buffers/images/samplers and draw materials. Preserve submesh transforms/names/groups/visibility, texture flags/channels/UV sets and ordinary load/reload/accessors.
2. Add typed draw constructors/setters/accessors to DrawableComponent while preserving production paths. Initialization selects one path; standard scene/bounds/clone consumers handle both explicitly.
3. Deep-clone CPU meshes and GPU mesh/material/texture realizations, including built tracing geometry, without default instancing or reuse of mutable clone resources through TextureCache.
4. Extend the existing TextureCache with draw overloads and minimal separate storage/context validation. Keep its current purpose; no ResourceManager or broad cache redesign.
5. Perform rare geometry/texture reloads at coordinated waitIdle boundaries. Keep mesh/texture objects out of resource rings; per-frame shader bindings are separate.

## Verification and exit conditions

- Editing an original after clone changes neither the clone CPU buffers nor its GPU realization/material/texture data.
- Production and draw component paths initialize correctly; invalid cross-mode/device access is reported and no native Vulkan resource escapes new draw objects.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M02 MaterialModifier/selection and M04 UV2 rebuild depend on clone/edit semantics. M09 deletes production overloads after new consumers are migrated.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
