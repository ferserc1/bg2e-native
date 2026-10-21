# M07 cumulative documentation closure

[Progress](../progress.md) · [Technical review](review.md) · [Documentation step](step_08_documentation.md)

Baseline HEAD: `78943d5820e69363c942d71136f479a0df87128e`; clean working tree at entry. No commit/staging/reset. The lead's next-step instruction accepts M07-07 and its delivered corrections under the continuation convention; compilation/checks are assumed successful under that convention, with backend/device/HTTP/package scenarios unspecified. This delivery changes Markdown only.

## Changed pages and source mappings

| Maintained pages / disposition | Actual implementation |
|---|---|
| [Composer guide](../../../../doc/apps/draw_composer.md), new; [app README](../../../../apps/draw/bg2e_composer/README.md), reconciled | `main.cpp`, `EditorUi.cpp`, `EditorActions.cpp`, `SceneEditor.cpp`, `StageScene.cpp`: target/backend/deployment, left Model Properties/right Scene, component intents, Chain, camera/light constraints, environment editing and complete IO. Removed contradictory startup-only/Environment Options/pending-batch README descriptions. |
| [Guided workflow](../../../../doc/tutorials/draw/composer.md), new | Actual launch/Open/import/create/select/edit/UV2/bake/Save sequence. No new numbered example or future standalone CLI. |
| [Draw import lifecycle](../../../../doc/apps/composer/draw_import_lifecycle.md), new; [HTTP guide](../../../../doc/apps/composer/import_service.md), clarified | `SceneImporter.cpp`, `ImportActions.cpp`, `ImportServer.cpp`, `DocumentIo.cpp`: separate request/document/renderer IDs, worker versus stopped realization, 16 waiting local jobs, one HTTP slot, canonical reimport rollback and shutdown. Production protocol guide retained with explicit draw implementation separation. |
| [Scene API](../../../../doc/api/scene/index.md), new; [scene reference IO](../../../../doc/api/db/SceneReferences.md), corrected | `Scene.hpp/.cpp`, `Node.cpp`, `ComponentFactory.cpp`, `CameraComponent.cpp`: root transfer, exact/subclass lookup, independent custom clones/projections, pure schema/resource hooks, weak explicit environment selection, malformed/unregistered skip versus registered failures. |
| [MainLoop API](../../../../doc/api/app/MainLoop.md) and [architecture](../../../../doc/architecture/MainLoop_render_draw_gpu.md), corrected | `MainLoop.cpp`: owned joinable workers, token-guarded completion and join before normal/exceptional cleanup. Removed obsolete detached-worker excerpt and lifetime claim. |
| [Scene editors](../../../../doc/api/ui/SceneEditors.md), [ResourcePicker](../../../../doc/api/ui/ResourcePicker.md), [safe updates](../../../../doc/safe_update_scene.md), updated | `SceneTree.hpp`, `ComponentInspector.hpp`, reflection PropertyMetadata, `ResourcePicker.cpp`, Composer edits: queued selection/reparent/add/remove/staged draw intents, resourceCanClear opt-in, cancelled Browse retention, Environment in-place edits, separate document/job validity. |
| [RendererDeferred](../../../../doc/api/draw/RendererDeferred.md), corrected | `SoftShadowPass.cpp`, query GLSL/Metal: current v5 persistent set2 arrays and reserve (2,1,10), neutral overflow with retained shadows. |
| [App index](../../../../doc/apps/index.md), [tutorial index](../../../../doc/tutorials/draw/index.md), [draw index](../../../../doc/api/draw/index.md), [DB index](../../../../doc/api/db/index.md), updated | Navigation to delivered Composer, scene API and guided workflow. |

## Retained cumulative coverage

M01–M06 documentation remains the delivered baseline, subject to the corrections above. No page is retired: production render/apps and the production standalone lightmap generator remain reference products through M08.

- [Complete draw IO](../../../../doc/api/db/DrawResourceIO.md) already covers parsed CPU entry points, synchronous realization, static formats, native GPU image archives, custom external-resource obligations and main-file-last publication. Composer's guide distinguishes per-item export and interactive adoption from DB transactions.
- [Environment](../../../../doc/api/draw/Environment.md) already covers explicit/null selection, enabled membership, root changes, ignored additions/removal/default, fresh-load regeneration, HDR alias/clearing, RGB retention and visible last-good resource errors. Model Edit's distinct environment workflow remains valid.
- [SelectableList](../../../../doc/api/ui/SelectableList.md), UI reference and quick-start recipe already guard items/endList with bool beginList. Retained after declaration/caller/recipe inspection; no new widget API is invented.
- [TracingSceneResources](../../../../doc/api/draw/TracingSceneResources.md), GPU array and effect references already describe the current persistent bindings/capacity policy. Source now uses tracing v3 and query shadows v5 rather than the older review's v2. Pending owner notes and operative plan decisions are reconciled. Native sampled arrays remain bounded by exact device and sampler budgets up to 16384; neutral overflow, immutable retained snapshots and independent object materials remain. Texture-count optimization stays deferred beyond migration.
- Draw/gpu/reflection/manipulation/utils foundations, tutorials 01–13, model editor tutorial, Workspace/window/input/delegate guides retain their scopes. Composer orchestrates those APIs without changing their signatures or production status.
- Bake context/executor/settings/output/UV preview references and integrated lightmap architecture retain completed-sample ownership and linear RGB UV2 semantics. Composer documentation supplies the app-specific serial scheduling, per-target preview, temporary PNG/HDR sources, finite half-range adoption restriction and partial dirty-adoption failure boundary.
- Existing app target registration, SDL bundle helper and packaging guidance remain. No CMake, source, shader or script changes, and no configure/build/install/package execution.

## Remaining owners and evidence

M08 owns standalone context/assembly, camera-less prefabs, batch protected-input/collision/overwrite policy, output publication and its CLI/application documentation. Composer does not deliver those APIs. M09 owns actual production deletion/promotion and canonical application/distribution reconciliation; M10 audits the maintained set. No missing Composer authoring is deferred to M10.

Static inspection checked declarations/call sites, ownership, source paths, current shader ABI/capacity arithmetic, Markdown local links and diff whitespace. Examples and CLI guidance were checked against source, not compiled or executed. No new application, GPU, HTTP, shader or package test is claimed. `skills.md` is absent at repository root; the supplied AGENTS instructions and actual declarations/review were used, and engine internals are unchanged.

Lead review: verify the guide matches current panels and Open/import/Environment/bake/Save usage; review API and worker/transaction limits. Any optional runtime regression should include deployed Metal/Vulkan resources, no-RT raster behavior, overflow diagnostics and native dialogs. M07-08 awaits lead acceptance. The next step is [M08-01](../milestone_08_lightmap_generator/step_01_standalone-context.md), conditional on that acceptance.
