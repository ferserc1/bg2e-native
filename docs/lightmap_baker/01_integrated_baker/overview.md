# Integrated lightmap baker

Create a reusable scene context and one baker per target Drawable inside an active windowed or offscreen application loop. The context owns production `bg2e::render::vulkan::rt::RayTracingScene` instances built from its scene root, one per in-flight frame slot. Do not call the scene/component lifecycle in this mode. Use existing UV2 meshes for this phase.

The required public names, ownership rules, frame-order example, RGB AO composition and validation rules are fixed in [API_CONTRACT.md](../API_CONTRACT.md). `prepareFrame(cmd, frame)` updates the context-owned bake TLAS once per active frame; each `IntegratedLightmapBaker::update(cmd, frame)` uses that prepared TLAS. The frame's RT scene is never used for baking, and no frame resource is retained. In model_edit, exclude editor-only floor geometry from the bake RT scene used for the model's isolated AO bake; it is not part of the model asset.

The per-target `UvSurfacePass` uses a configured, depthless `GBufferManager` for its own UV attachments. It does not reuse the `DeferredLayer` camera G-buffer images or shaders.

**Phase completion:** both editors can bake an existing usable UV2 atlas to a temporary image, attach it to AO on all target submeshes, preview the result and save using the existing asset-copy path. Their active application loops continue to own all scene lifecycle calls.

## Ordered steps

1. [Define the public contract](step-01_api_contract.md)
2. [Build the UV-space surface pass](step-02_uv_surface.md)
3. [Build the context-owned bake RT scene](step-03_scene_bindings.md)
4. [Implement RTAO baking](step-04_rtao.md)
5. [Implement RTGI and optional RT shadows](step-05_rtgi_shadows.md)
6. [Add accumulation and CPU/GPU results](step-06_accumulation_output.md)
7. [Integrate into model_edit and bg2e_composer](step-07_application_integration.md)
8. [Document integrated baking](step-08_documentation.md)

Every step has a compile gate. The penultimate step integrates the relevant application(s); the final step updates permanent documentation in doc/. See [plan-wide decisions](../README.md).
