# Step 03: Assemble model and prefab scenes

## Scope

Provide an input assembly path for a JSON lighting context plus either one .bg2 model at the origin or a JSON prefab subtree. Attach the target nodes to the context scene before updating it, so all context and target geometry participate in the TLAS and inter-occlude. Validate missing assets and ownership; preserve input files untouched.

## Acceptance and compile gate

Both modes assemble a single scene and enumerate the intended target Drawables. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_04_image_formats.md](prestep_04_image_formats.md) for the next step (Add image-format discovery in db).
