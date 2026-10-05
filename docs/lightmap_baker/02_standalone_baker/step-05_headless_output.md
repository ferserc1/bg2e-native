# Step 05: Define standalone output policy

## Scope

Add a reusable result writer that converts RGB8 (or float result to RGB8 when requested) to the selected stb format. For prefab, write one image per valid target and write target .bg2 copies only when UV2 generation is requested; for model, write one image and copy the .bg2 only when requested. Never overwrite input resources. Define collision-safe output names from stable node identities and explicit failure on unresolved collisions. Do not emit a new prefab/context JSON.

## Acceptance and compile gate

A dry-run or fixture validates paths and preserves input files; all generated files are confined to the output directory. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_06_batch_execution.md](prestep_06_batch_execution.md) for the next step (Implement standalone batch control).
