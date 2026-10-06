# Standalone lightmap baker

Extend the shared baking core for command-line execution without an active application loop. A standalone context owns its headless frame resources and its own production `RayTracingScene`, drives the required scene/component lifecycle, then bakes multiple targets. This RT scene is distinct from the integrated context's per-slot owned instances; neither mode borrows the ordinary renderer's frame TLAS. Add apps/lightmap_generator with model and prefab modes.

The public lifecycle and exact CLI examples are in [API_CONTRACT.md](../API_CONTRACT.md). One `StandaloneBakerContext` assembles one context scene plus all targets, calls `updateScene` once, and creates a separate `StandaloneLightmapBaker` per target. No window, camera or active application loop is required.

**Phase completion:** the CLI runs model and prefab batches with usable pre-existing UV2 and writes images only. It accepts the UV2-generation flag but reports that the true branch becomes available in phase 3. The `false` branch is fully functional.

## Ordered steps

1. [Define standalone lifecycle](step-01_lifecycle_contract.md)
2. [Drive scene updates and own the TLAS](step-02_scene_update.md)
3. [Assemble model and prefab scenes](step-03_input_assembly.md)
4. [Add image-format discovery in db](step-04_image_formats.md)
5. [Define standalone output policy](step-05_headless_output.md)
6. [Implement standalone batch control](step-06_batch_execution.md)
7. [Create apps/lightmap_generator](step-07_application_integration.md)
8. [Document standalone baking](step-08_documentation.md)

Every step has a compile gate. The penultimate step integrates the relevant application(s); the final step updates permanent documentation in doc/. See [plan-wide decisions](../README.md).
