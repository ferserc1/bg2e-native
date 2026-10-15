# M05-04 — Build examples/draw/13_offscreen_application

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M05-01–M05-03.

## Scope and deliverable

- `examples/draw/10_deferred_renderer/src/main.cpp`
- `examples/draw/13_offscreen_application/src/main.cpp`
- `examples/draw/13_offscreen_application/CMakeLists.txt`
- `examples/CMakeLists.txt`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [examples/CMakeLists.txt](../../../../examples/CMakeLists.txt).

## Implementation work

1. Reuse equivalent scene construction/assets/materials/lights/environment from the deferred example without copy-pasting engine execution. Disable overlays and freeze its rotating scene by default.
2. Expose --output-dir, output width/height, internal resolution or scale, --frames, --backend and --graphics-config, with documented validation/precedence.
3. Create one named output file after the accumulated result is complete; support deterministic destination diagnostics and useful process exit codes.
4. Register a unique CLI target and paired shader/asset packaging; no hidden window or dependency on an interactive application is allowed.

## Verification and exit conditions

- The exact requested destination contains one valid image with expected dimensions and scene content; jobs complete at N rendered samples.
- The example runs with reduced resolution and no RT/FSR, plus supported advanced effects, using a settings export from the interactive example.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M08 CLI uses the same application lifecycle/argument style but bakes UV-space maps instead of camera color.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-15 source scene

Use examples/draw/10_deferred_renderer and [working-example.md](../milestone_01_deferred_renderer/working-example.md) as the actual scene/material/settings baseline. Its window/retained presentation and FSR SDK context do not fulfill offscreen initialization, accumulated scene sampling or file export. Reuse resources/settings while implementing those responsibilities here.
