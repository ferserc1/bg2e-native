# Handoff for Create apps/lightmap_generator

Next implementation step: [step-07_application_integration.md](step-07_application_integration.md). Complete this file **after** finishing step 06; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-07_application_integration.md.

## Next-step instructions

Create the command-line executable with model and prefab modes, required JSON context, output directory, stb image format, UV2-generation flag, square resolution, quality/accumulation frames and relevant GI/AO/shadow options. Parse/validate arguments, initialize headless engine resources, load and assemble inputs, run the standalone context and write outputs. Add the app to the build configuration as explicitly required by this new application. At this stage, the UV2 flag can report that regeneration becomes available in phase 3; preserve compilation and working no-regeneration flows.

## Next-step acceptance gate

The executable builds and performs model/prefab batches with existing UV2; invalid input returns a nonzero code and useful diagnostics. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.
