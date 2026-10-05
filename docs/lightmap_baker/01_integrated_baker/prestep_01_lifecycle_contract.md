# Handoff for Define standalone lifecycle

Next implementation step: [../02_standalone_baker/step-01_lifecycle_contract.md](../02_standalone_baker/step-01_lifecycle_contract.md). Complete this file **after** finishing step 08; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in ../02_standalone_baker/step-01_lifecycle_contract.md.

## Next-step instructions

Add StandaloneBakerContext as a distinct public type sharing only the bake core with IntegratedBakerContext. Specify initialize, resize when required, updateScene, bake and cleanup ordering. Define the scene generation/version that invalidates every affected baker history. Do not trigger lifecycle calls from integrated mode.

## Next-step acceptance gate

Headers and implementation stubs compile; invalid call order reports clear errors. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.
