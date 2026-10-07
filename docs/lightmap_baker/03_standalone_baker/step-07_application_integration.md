# Step 07: Create apps/lightmap_generator

## Scope

Create the command-line executable with model and prefab modes, required JSON context, output directory, stb image format, UV2-generation flag, square resolution, quality/accumulation frames and relevant GI/AO options. Parse/validate arguments, initialize headless engine resources, load and assemble inputs, run the standalone context and write outputs. Add the app to the build configuration as explicitly required by this new application. Both UV2 flag branches must work in this step using the phase-2 modifier and validator.

## Acceptance and compile gate

The executable builds and performs model/prefab batches with existing UV2; invalid input returns a nonzero code and useful diagnostics. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Create `apps/lightmap_generator` with a thin `main.cpp`: parse the exact flags in [API_CONTRACT.md](../API_CONTRACT.md), initialize `render::Engine` headlessly, assemble inputs, call the standalone batch and write outputs. Keep loading/baking/output logic in reusable classes rather than a monolithic main function.
- Required options: subcommand `model` or `prefab`, `--context`, `--model` or `--prefab`, `--output`. Common options: `--format`, `--resolution`, `--frames`, `--mode`, `--generate-uv2`, `--samples-per-pixel`, `--gi-bounces`, `--max-distance`. Include `--help`. Defaults are those in `LightmapSettings`; default image format PNG. The distance option applies only to RTGI; RTAO uses a fixed 0.1 m radius.
- Reject missing, duplicate, contradictory or irrelevant options before initializing Vulkan. With `--generate-uv2=true`, regenerate every target atlas before its final GPU/BLAS load and write a new .bg2 plus image per target. With `false`, validate existing UV2, write images only and warn once per skipped target. Model and prefab batches must both complete. Log per-target warnings and final summary to stdout/stderr with nonzero exit on fatal error.
- Add the new app target to `apps/CMakeLists.txt` and its own CMake file during the future implementation request. Preserve Linux Ninja, macOS Xcode and Windows Visual Studio build rules; no packaging change is needed unless separately requested.

## CLI example

```sh
lightmap_generator prefab --context studio.json --prefab sofa.json \
  --output out --format png --resolution 512 --frames 32 \
  --mode rtgi --generate-uv2=false \
  --samples-per-pixel 8 --gi-bounces 2 --max-distance 50
```

## Handoff

After this step, complete [prestep_08_documentation.md](prestep_08_documentation.md) for the next step (Document standalone baking).
