# Handoff for Define standalone output policy

Next implementation step: [step-05_headless_output.md](step-05_headless_output.md). Complete this file **after** finishing step 04; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-05_headless_output.md.

## Next-step instructions

Add a reusable result writer that converts RGB8 (or float result to RGB8 when requested) to the selected stb format. For prefab, write one image per valid target and write target .bg2 copies only when UV2 generation is requested; for model, write one image and copy the .bg2 only when requested. Never overwrite input resources. Define collision-safe output names from stable node identities and explicit failure on unresolved collisions. Do not emit a new prefab/context JSON.

## Next-step acceptance gate

A dry-run or fixture validates paths and preserves input files; all generated files are confined to the output directory. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `db::LightmapOutputWriter` taking `LightmapPixels`, `db::ImageFormat`, output directory and a stable target identity. Convert float CPU values to RGB8 with one clamp/round if a float result was requested; call the existing `db::saveImage` writer for PNG/JPEG/BMP/TGA. The CLI itself is not created until the penultimate step.
- Preflight all output paths for the entire batch. Use a stable sanitized node path plus a deterministic suffix for duplicate display names; fail if two final paths still collide, if an output path aliases any input resource, or if an output file already exists. Write via temporary sibling files that retain the real image extension (for example `sofa.pending.png`, because stb dispatches on extension), then rename only after success so a failed bake leaves no partial final image. Never write a context/prefab JSON.
- Plan and implement both output branches using the phase-2 modifier: with UV2 regeneration, write a fresh .bg2 copy for each target and set its AO material path to the generated image; without it, write only images and do not save a model. The CLI wires these branches to its flag in step 07.
- Reuse the same path planning for model (one target) and prefab (all eligible target nodes). Do not modify loaded input file paths in-place.
