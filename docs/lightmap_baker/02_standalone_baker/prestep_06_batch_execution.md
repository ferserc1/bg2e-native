# Handoff for Implement standalone batch control

Next implementation step: [step-06_batch_execution.md](step-06_batch_execution.md). Complete this file **after** finishing step 05; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-06_batch_execution.md.

## Next-step instructions

Implement full-resolution bake orchestration for all selected targets using one standalone context and one updateScene after complete input assembly. Support square resolution, RTAO/RTGI, shadows, samples per pixel, GI bounces, maximum distance and exact accumulation-frame count. Skip and warn for targets lacking usable UV2 when regeneration is disabled. Provide a cancellable progress callback for CLI logging, separate from the deferred GUI progress feature.

## Next-step acceptance gate

A batch can process multiple targets without rebuilding the TLAS per target, and reports skipped targets. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.
