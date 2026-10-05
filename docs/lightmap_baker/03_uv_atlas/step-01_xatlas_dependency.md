# Step 01: Introduce xatlas under MIT

## Scope

Vendor a pinned xatlas release with its MIT LICENSE and provenance. Add its source to the engine build only as necessary for compilation; do not alter unrelated CMake logic. Wrap the dependency behind geo-facing types so public headers do not expose xatlas. Record the single-atlas requirement and error handling.

## Acceptance and compile gate

The engine links xatlas and otherwise behaves as before. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_02_atlas_modifier.md](prestep_02_atlas_modifier.md) for the next step (Generate one atlas across submeshes).
