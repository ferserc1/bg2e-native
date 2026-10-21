# Compose, light and save a scene

Use the delivered `bg2e_composer` with its deployed shaders/assets. Launch with `--backend metal` on a supported macOS host or `--backend vulkan` where available. See [application/deployment guidance](../../apps/draw_composer.md).

1. Choose New Scene, or Open an existing camera-bearing scene. While Open shows the Loader, wait for candidate adoption. If it fails, read the visible error; the prior document remains. Save/Discard/Cancel handles pending edits.
2. Use Create Node or Create Primitive, or Import Model or Prefab. Import inserts below the selected node or document root. Keep source textures available while parsing/realization completes. Cancel prevents late insertion.
3. Select nodes in the right Scene hierarchy. Shift-click extends selection. Drag a node onto another to reparent with its local transform retained. Edit reflected properties or use Add Component/Remove; keep an enabled camera and no more than eight enabled lights. Select submeshes in the left Model Properties panel to edit materials. Chain controls edit model joints.
4. Select the active Environment node. Change procedural RGB with no HDR assigned, then Browse for a floating-point HDR image. Clear returns to stored colors; cancelling Browse keeps the current source. Additional Environment components remain inactive while one is assigned. Removing the active one restores the default sky. Generation errors appear in status/inspector.
5. Open the UV/Lightmap window. Generate missing UV2, select preview/selected-subtree/whole-scene targets and start baking. Keep sources unchanged until all targets finish. Cancel if needed; unsupported required RT reports failure rather than silently baking a different effect. Inspect each completed target preview after automatic material application.
6. Save As to publish the scene with actual model/texture resources. Reopen the saved document and check hierarchy, camera, materials and linear UV2 baked maps. Move the generated sidecars together with the main file. Export Selected Model(s) instead exports only directly selected models and joints, without placement or descendants.

For external-tool imports, configure Window → Import Settings and use the [localhost protocol](../../apps/composer/import_service.md). A busy HTTP request occupies one slot; local imports have their own queue. A second application cannot bind the same port.

This source-checked workflow is not a new executed backend/package test. The project lead owns runtime checks. See the application guide for partial material-adoption failures, temporary source ownership and complete-save limits.
