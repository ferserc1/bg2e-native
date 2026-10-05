# Standalone lightmap baker

Extend the shared baking core for command-line execution without an active application loop. A standalone context owns frame resources and the production TLAS, drives the required scene/component lifecycle, then bakes multiple targets. Add apps/lightmap_generator with model and prefab modes.

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
