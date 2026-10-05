# Integrated lightmap baker

Create a reusable scene context and one baker per target Drawable inside an active windowed or offscreen application loop. Reuse the production bg2e::render::vulkan::rt::RayTracingScene for the current frame. Do not call the scene/component lifecycle in this mode. Use existing UV2 meshes for this phase.

The required public names, ownership rules, frame-order example, RGB AO composition and validation rules are fixed in [API_CONTRACT.md](../API_CONTRACT.md). The frame's TLAS is borrowed on each `IntegratedLightmapBaker::update(cmd, frame)` call. No frame resource may be retained. In model_edit, exclude editor-only floor geometry from the RT scene used for the model's isolated AO bake; it is not part of the model asset.

**Phase completion:** both editors can bake an existing usable UV2 atlas to a temporary image, attach it to AO on all target submeshes, preview the result and save using the existing asset-copy path. Their active application loops continue to own all scene lifecycle calls.

## Ordered steps

1. [Define the public contract](step-01_api_contract.md)
2. [Build the UV-space surface pass](step-02_uv_surface.md)
3. [Reuse frame RT data safely](step-03_scene_bindings.md)
4. [Implement RTAO baking](step-04_rtao.md)
5. [Implement RTGI and optional RT shadows](step-05_rtgi_shadows.md)
6. [Add accumulation and CPU/GPU results](step-06_accumulation_output.md)
7. [Integrate into model_edit and bg2e_composer](step-07_application_integration.md)
8. [Document integrated baking](step-08_documentation.md)

Every step has a compile gate. The penultimate step integrates the relevant application(s); the final step updates permanent documentation in doc/. See [plan-wide decisions](../README.md).
