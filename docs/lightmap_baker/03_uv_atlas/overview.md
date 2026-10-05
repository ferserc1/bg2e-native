# UV atlas generator

Add MIT-licensed xatlas-backed UV2 generation for complete meshes, then integrate it into both editors and the CLI. UV1 must remain bitwise unchanged for each original vertex/corner. The modifier is CPU-only; application code performs safe GPU reloads.

## Ordered steps

1. [Introduce xatlas under MIT](step-01_xatlas_dependency.md)
2. [Generate one atlas across submeshes](step-02_atlas_modifier.md)
3. [Validate and visualize atlas data](step-03_uv_validation.md)
4. [Add UI texture preview without exposing ImGui](step-04_ui_preview.md)
5. [Plan safe editor mesh replacement](step-05_safe_reload.md)
6. [Integrate UV2 generation and previews](step-06_application_integration.md)
7. [Document UV2 generation and previews](step-07_documentation.md)

Every step has a compile gate. The penultimate step integrates the relevant application(s); the final step updates permanent documentation in doc/. See [plan-wide decisions](../README.md).
