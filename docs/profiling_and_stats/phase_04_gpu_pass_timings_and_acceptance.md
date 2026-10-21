# Phase 04 GPU pass timings and acceptance

## Outcome

Performance provides a supported GPU pass breakdown alongside CPU stages and counters. Recording and live statistics are validated across lifecycle changes, unsupported capabilities and sustained workloads, with measured overhead and explicit remaining platform gaps.

## Scope and implementation

1. Extend the phase 03 GPU facility with bounded named intervals and hierarchy/parent metadata. Match begin/end requests across normal and exceptional recording paths. Report missing endpoints, capacity overflow and unsupported sampling points explicitly.
2. Implement Vulkan interval timestamps at legal graphics/compute/transfer boundaries. Define the pipeline-stage interpretation and avoid unnecessary barriers or command-buffer splits solely for measurement. Preserve dynamic rendering validity and query lifetime rules.
3. Implement Metal pass/encoder timing using supported counter sets and sampling points. Query capabilities at runtime; resolve counter data only after normal completion and convert native timestamps with a documented supported timebase/calibration method. Do not require unsupported per-draw/per-dispatch sampling or force encoder splits for apparent precision.
4. Keep phase 03 total command-buffer timing as a complete fallback. If a Metal device supports only coarse encoder/pass timing, display that granularity. Never fabricate fine-grained intervals from CPU scope times or divide an encoder duration among its dispatches.
5. Instrument selected draw boundaries: opaque G-buffer, deferred lighting, transparent processing, query shadows, AO/GI/reflections, temporal accumulation, denoise, reconstruction, final conversion and UI composition where legal. Begin with coarse scopes and enable detailed transparent-layer/effect intervals optionally to bound overhead and query count.
6. Correlate GPU intervals with the existing CPU labels and work counters using stable pass/context identities. Preserve repeated invocation/layer identity, include backend/capability metadata and distinguish whole-pass elapsed time from any vendor-specific stage metric. Do not sum nested or overlapping GPU intervals.
7. Extend the Performance stage selector/table and recording schema implementation for available pass intervals. Provide explicit unavailable/partial/pending states and surface dropped samples. Public headers remain free of ImGui and native backend types.
8. Measure and document overhead for disabled, live CPU, recording, total GPU and detailed GPU modes. Report collection, snapshot publication, chart rendering, writer and query costs separately where possible. Bound memory under long sessions, high label cardinality and many transparent layers.
9. Complete error/lifecycle handling: file write failure, queued GPU results at Stop, hidden-window recording, device loss, callbacks after owner destruction, resize, scene/renderer replacement and application exit. Do not introduce synchronization merely to make reports complete.

## Compilation boundary

- Implement each public interval API and both backend branches before enabling draw call sites. Coarse-only and unsupported devices remain functional in the same source tree.
- Deliver chart/table/schema consumers and capability descriptions with the instrumentation that feeds them.
- No public ImGui headers/types, production render migration, new external dependency or CMake modification is part of this phase.

## Verification and exit

Run the functional matrix from the summary on available Metal/Vulkan devices, including CPU-only and coarse-GPU fallback. Check that frame/session identities survive delayed completion and that unrelated intervals are never attributed to the current scene/frame.

Compare supported intervals with native graphics tooling. Record backend, hardware, capabilities, resolution, scene size, transparent layers, active effects, sampling detail and timing granularity. Compare identical settled workloads with profiling disabled/enabled, separating warm-up and transition frames. Verify memory limits and explicit omission counts during sustained captures.

Build the library and affected applications and run native validation only when authorized. Audit exported headers/transitive includes for ImGui leakage and exercise the composer recording workflow end to end. Unavailable hardware, unrun compilation and missing native comparisons remain explicit gaps; do not label them passed.

Exit with all four phases functional, files readable and finalized, unsupported cases explained, no measurement-induced GPU waits, and a completion record containing actual validation and overhead evidence.
