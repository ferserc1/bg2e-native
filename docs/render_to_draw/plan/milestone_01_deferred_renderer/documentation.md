# M01-17 cumulative documentation closure

Date: 2026-10-15. Source baseline: `4d58ce60f568dca2e977b27ae5161a8b94913dbd`.
M01-16 explicitly validated by the project lead in this chat. The lead explicitly
waived a separate documentation validation and authorized M02-01 immediately
after this closure. That direction overrides the ordinary one-step handoff here.

## Inventory and source mapping

| Maintained pages | Disposition and source coverage |
|---|---|
| doc/api/draw/index.md, reference.md, quick_start.md, RenderLoopDelegate.md, RenderLoop.md, FrameContext.md | Updated navigation/scope and base/runtime delegate distinction; draw Engine/RenderLoop/DefaultRenderLoopDelegate sources and Application/MainLoop callers. |
| doc/api/draw/RendererDeferred.md | Added lifecycle, retained scene/replacement/generations, public RenderLayer contracts, deferred graph/query limits and explicit transparency deferral; Renderer.hpp, RendererDeferred.hpp/.cpp and pipeline sources. |
| doc/api/draw/Resources.md | Added Drawable/material/texture clone ownership, dual component modes, draw visitors/bindings, UV/packed channels and safe edits; draw resource sources, scene/DrawableComponent and utils/TextureCache. |
| doc/api/draw/Environment.md | Added public environment family, snapshots, generation, color/mip semantics and accepted HDR precision correction; environment headers/sources and Texture.cpp. |
| doc/api/draw/Settings.md | Added live catalog, typed codec, optional persistence/facade/UI generations, resolution/open upscaler and actual FSR limits; SettingsCatalog/SettingsCodec/RenderSettingsPreferences/Upscaler/FsrUpscaler and GraphicsSettingsWindow. |
| doc/api/gpu/Subresources_and_rendering.md and linked Image/CommandBuffer/GraphicsPipeline/ResourceSet/PhysicalDevice/index/reference pages | Added MRT/state, capabilities, explicit native bindings, ranges/views/layouts, upload/mips/copies/raw color readback and unsupported depth/array cases; GPU common and both backend implementations. |
| doc/api/db/SceneReferences.md, index.md, reference.md | Added raw/typed/reference IO versus Engine realization and actual OBJ/BG2 subset; db/scene, mesh_obj, mesh_bg2 and Component schema hooks. |
| doc/tutorials/draw/10_deferred_renderer.md, index.md; doc/apps/draw_deferred_renderer.md, index.md | Added usable setup/CLI/workflow, public injection, no-RT/SDK fallbacks, registered target/resources/generator/distribution guidance; actual numbered example and scripts/PACKAGING.md. |
| doc/api/ui/Settings_Windows.md, Reflection_Widgets.md; doc/api/reflection/reference.md | Updated draw live UI/proxy references, retaining production UI behavior. |
| doc/architecture/MainLoop_render_draw_gpu.md, doc/deferred_render_model.md, doc/delegate_system.md, doc/safe_update_scene.md, doc/api/app/MainLoop.md | Reconciled current draw scene lifecycle and linked new canonical contracts while retaining production/conceptual material. |
| Other draw Engine/EngineConfig pages, GPU foundation/tutorials, app/input/window/workspace, geo/math/JSON/reflection foundation, production render/lightmap/app guides | Retained: foundation APIs and production workflows continue. Base loop color-target scope does not exclude renderer-owned internal depth/G-buffer. Production bake/advanced RT/editor docs do not claim draw equivalents. |

No maintained page was retired. These are canonical updates, not another API
snapshot. Future manipulation/UI/material tools belong to M02 documentation,
advanced effects/history to M03, bake to M04, windowless/export to M05, complete
IO/apps to M06–M08, canonical production retirement to M09 and final audit to M10.
No unimplemented workflow is documented as available.

## Checks and limits

Compared public declarations with actual example caller, reviewed lifecycle,
settings types, resource domains, HDR correction and query/FSR limitations. Local
Markdown path checks pass (C++ lambda syntax excluded from link interpretation);
source/header/target/resource references and setup snippet checked statically.
No compiler, shader compiler, executable, packaging script or runtime image check
was run. Prior project-lead acceptance is retained, with backend coverage
unspecified. Transparency remains explicitly deferred. Existing architecture
source excerpts are historical abbreviated excerpts; the current renderer and
base-hook references take precedence for additions.

M01 documentation is complete and accepted by the lead's explicit advance
authorization; M02-01 may begin. No claim of new runtime validation is made.
