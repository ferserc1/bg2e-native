# M02-06 — Experimental optimal-viewpoint tools in utils

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M02-02 and M01 draw visitors/resources.

## Scope and deliverable

- `lib/include/bg2e/render/ViewpointAnalyzer.hpp`
- `lib/src/bg2e/render/ViewpointAnalyzer.cpp`
- `lib/include/bg2e/utils/SceneCameraUtils.hpp`
- `lib/include/bg2e/utils/ViewpointAnalyzer.hpp`
- `lib/src/bg2e/utils/ViewpointAnalyzer.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/ViewpointAnalyzer.hpp](../../../../lib/include/bg2e/render/ViewpointAnalyzer.hpp), [lib/src/bg2e/render/ViewpointAnalyzer.cpp](../../../../lib/src/bg2e/render/ViewpointAnalyzer.cpp), [lib/include/bg2e/utils/SceneCameraUtils.hpp](../../../../lib/include/bg2e/utils/SceneCameraUtils.hpp).

## Implementation work

1. Implement the new analyzer/candidate/range/statistic API in utils using draw/GPU measurement resources. Leave the current production ViewpointAnalyzer implementation untouched.
2. Reuse ID/depth drawing/readback where behavior matches; add typed depth readback in GPU if required after comparing native versus higher-level complexity.
3. Preserve candidate sampling/cyclic ranges/bounds fitting, border/coverage/depth/per-submesh statistics and overridable score behavior. Add camera-application overloads for the new candidate type.
4. Expose ordinary utility configuration and graphics ownership; no new private tool class unless essential. Keep application-specific presets out of GPU.

## Verification and exit conditions

- On an equivalent scene, candidate generation and ID statistics are inspectable and sensible; the new utility can run in draw mode without production resources.
- Empty scene, off-frustum candidates and multiple submesh identities have defined results; legacy analyzer behavior is unchanged.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M06/M07 integrate optimal framing UI. M05 can later reuse measurement offscreen but no headless application is implemented here.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
