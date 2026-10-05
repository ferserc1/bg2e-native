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
