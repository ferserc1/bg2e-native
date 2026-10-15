# M07-06 — Composer feature parity and complete-scene regression

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M07-01–M07-05.

## Scope and deliverable

- `apps/draw/bg2e_composer/`
- `apps/bg2e_composer/`
- `doc/apps/composer/import_service.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/bg2e_composer/](../../../../apps/bg2e_composer), [doc/apps/composer/import_service.md](../../../../doc/apps/composer/import_service.md).

## Implementation work

1. Verify every inventoried workflow against equivalent current composer behavior: graph editing/clones/serialization, assets/import service, materials/selection, camera tools, settings and baking.
2. Test packaged assets/shaders/native dialogs, error/unsaved prompts and cleanup/worker shutdown on relevant platforms.
3. Exercise no-RT/default-scale and supported RT/FSR modes, capability-driven option visibility and configuration-file round-trip.
4. Record intentional differences and missing parity; do not postpone a lost application feature to removal milestone.

## Verification and exit conditions

- The new target operates from a deployed layout and scenes round-trip with resources and the expected hierarchy.
- Required application workflows have evidence and production app/examples remain functioning before retirement.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M08/M09 gates use this evidence to decide whether production scene/app code is no longer needed.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
