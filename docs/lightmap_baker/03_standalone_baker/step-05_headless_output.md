# Step 05: Define standalone output policy

## Scope

Add a reusable result writer that converts RGB8 (or float result to RGB8 when requested) to the selected stb format. For prefab, write one image per valid target and write target .bg2 copies only when UV2 generation is requested; for model, write one image and copy the .bg2 only when requested. Never overwrite input resources. Define collision-safe output names from stable node identities and explicit failure on unresolved collisions. Do not emit a new prefab/context JSON.

## Acceptance and compile gate

A dry-run or fixture validates paths and preserves input files; all generated files are confined to the output directory. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add `db::LightmapOutputWriter` taking `LightmapPixels`, `db::ImageFormat`, output directory and a stable target identity. Convert float CPU values to RGB8 with one clamp/round if a float result was requested; call the existing `db::saveImage` writer for PNG/JPEG/BMP/TGA. The CLI itself is not created until the penultimate step.
- Preflight all output paths for the entire batch. Use a stable sanitized node path plus a deterministic suffix for duplicate display names; fail if two final paths still collide, if an output path aliases any input resource, or if an output file already exists. Write via temporary sibling files that retain the real image extension (for example `sofa.pending.png`, because stb dispatches on extension), then rename only after success so a failed bake leaves no partial final image. Never write a context/prefab JSON.
- Plan and implement both output branches using the phase-2 modifier: with UV2 regeneration, write a fresh .bg2 copy for each target and set its AO material path to the generated image; without it, write only images and do not save a model. The CLI wires these branches to its flag in step 07.
- Reuse the same path planning for model (one target) and prefab (all eligible target nodes). Do not modify loaded input file paths in-place.

## Output example

`out/sofa_arm.png` and, only with UV2 generation, `out/sofa_arm.bg2`; the input prefab JSON is never written.

## Handoff

After this step, complete [prestep_06_batch_execution.md](prestep_06_batch_execution.md) for the next step (Implement standalone batch control).
