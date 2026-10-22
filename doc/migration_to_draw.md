# Migrating to production draw

The previous rendering namespace, native wrappers, forward/MSAA modes and old
application targets have been removed. This is a source/ABI break with no aliases
or old-binary compatibility. Use `<bg2e/all.hpp>` or explicit draw/bake umbrellas.

| Previous usage | Current contract |
|---|---|
| Templated graphics delegate | Non-template draw::DefaultRenderLoopDelegate with an exclusively owned Renderer. |
| Framework/execution mode | Both MainLoop run overloads use draw; EngineConfig chooses the GPU backend. |
| Scene-native Drawable/resources | draw::Drawable, DrawableComponent::drawable() and canonical shared ownership. |
| Multiple factory creation paths | ComponentFactoryRegistry registration by name and default creator. |
| Native renderer baking | bake contexts, public UV passes, serial standalone batches and LightmapOutputWriter. |
| Old app folders/targets | `apps/model_edit`, `apps/bg2e_composer` and `apps/lightmap_generator`, with matching target names. |
| Flat shader output | Paired draw/bake runtime trees and explicit native shader contracts. |

Preserve authored Drawable component identities and CPU data concepts; realize
resources through a draw Engine. Deep clones retain independent mesh/material/
texture state, while material() after loading is a live alias. Safe rare edits
must stop producers and coordinate device completion. Scene saves/resource exports
persist CPU model data and assigned standard image resources and report bool-noexcept failure;
keep parents, rollback and no-clobber behavior.

Settings use current stable component/property identities and strict typed payloads.
Unknown/unavailable properties reject by default; skipping is explicit. Application
preference IDs and import-service protocol identities remain unchanged. Output
layouts and settings documents must follow the current CLI/API contracts rather
than assuming an old executable or serialized configuration is compatible.

Continue with [API](api/index.md), [tutorials](tutorials/draw/index.md),
[applications](apps/index.md) and [build/deployment](architecture/build_and_distribution.md).
