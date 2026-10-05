# Step 07: Create apps/lightmap_generator

## Scope

Create the command-line executable with model and prefab modes, required JSON context, output directory, stb image format, UV2-generation flag, square resolution, quality/accumulation frames and relevant GI/AO/shadow options. Parse/validate arguments, initialize headless engine resources, load and assemble inputs, run the standalone context and write outputs. Add the app to the build configuration as explicitly required by this new application. At this stage, the UV2 flag can report that regeneration becomes available in phase 3; preserve compilation and working no-regeneration flows.

## Acceptance and compile gate

The executable builds and performs model/prefab batches with existing UV2; invalid input returns a nonzero code and useful diagnostics. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_08_documentation.md](prestep_08_documentation.md) for the next step (Document standalone baking).
