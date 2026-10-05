# Step 06: Implement standalone batch control

## Scope

Implement full-resolution bake orchestration for all selected targets using one standalone context and one updateScene after complete input assembly. Support square resolution, RTAO/RTGI, shadows, samples per pixel, GI bounces, maximum distance and exact accumulation-frame count. Skip and warn for targets lacking usable UV2 when regeneration is disabled. Provide a cancellable progress callback for CLI logging, separate from the deferred GUI progress feature.

## Acceptance and compile gate

A batch can process multiple targets without rebuilding the TLAS per target, and reports skipped targets. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_07_application_integration.md](prestep_07_application_integration.md) for the next step (Create apps/lightmap_generator).
