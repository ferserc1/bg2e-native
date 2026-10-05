# Step 08: Document integrated baking

## Scope

Update doc/api/render with context, baker, settings, result and lifetime contracts. Add doc/api/ui references for both windows, a technical description in doc/ when useful, and UV2 fixture instructions. Describe RGB8/float output, AO-slot meaning, frame ordering, and no scene lifecycle calls in integrated mode.

## Acceptance and compile gate

Documentation links resolve and match the implemented public API. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_01_lifecycle_contract.md](prestep_01_lifecycle_contract.md) for the next step (Define standalone lifecycle).
