# Handoff for Assemble model and prefab scenes

Next implementation step: [step-03_input_assembly.md](step-03_input_assembly.md). Complete this file **after** finishing step 02; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-03_input_assembly.md.

## Next-step instructions

Provide an input assembly path for a JSON lighting context plus either one .bg2 model at the origin or a JSON prefab subtree. Attach the target nodes to the context scene before updating it, so all context and target geometry participate in the TLAS and inter-occlude. Validate missing assets and ownership; preserve input files untouched.

## Next-step acceptance gate

Both modes assemble a single scene and enumerate the intended target Drawables. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.
