# Temporal accumulation implementation progress

## Resume pointer

- Plan: [summary.md](summary.md)
- Current phase: 0
- Next step: **P0.1** in [phase_0_preserve_history.md](phase_0_preserve_history.md)
- State: `not_started`
- Last completed implementation step: none
- Next exact action: read AGENTS.md, inspect git status, search for skills.md, then audit the signature/fingerprint callers and record the Phase 0 file list before editing engine code.

## Authorization and workspace

- Authorized now: writing this implementation plan and progress file only. Implementation follows when requested by the user.
- Compilation/shader compilation: not authorized by the plan-writing request; do not run.
- Native runtime validation/benchmarks: not run.
- CMake changes: prohibited unless explicitly requested.
- skills.md: not found during planning; search again before implementation.
- Existing unrelated changes: numerous README/API/architecture files and tests README files were modified before plan creation. Preserve them. Record a fresh git status at implementation start.

## Fixed decisions

- Object-motion trails are accepted in Phase 0; no global reset for transform-only edits, including fallback paths.
- Keep full transform-sensitive scene fingerprints and exact cached/reference serialization parity.
- Freeze preference remains unchanged; temporarily roll during motion and for 3 * maxFrames subsequent rendered scene frames.
- No changes to maxFrames for thawing; no invalidating setters during interaction.
- Stable CPU key is nodeId/submesh; GPU token is a nonzero monotonic uint32.
- Separate metadata raster pass; preserve the six-color G-buffer and depth index.
- Phase 0 camera reset policy remains unchanged. Phase 2 enables continuous-camera reprojection on supported built-in paths.
- Global material/light/environment/topology/membership reset policy remains unless explicitly scoped otherwise in a later approved change.
- Deferred: deformation history, virtual reflection motion, adaptive ray counts, external denoiser integration.

## Phase ledger

| Phase | Implementation | Source checks | Vulkan runtime | Metal runtime |
|---|---|---|---|---|
| 0 | not_started | not_run | not_run | not_run |
| 1 | not_started | not_run | not_run | not_run |
| 2 | not_started | not_run | not_run | not_run |
| 3 | not_started | not_run | not_run | not_run |
| 4 | not_started | not_run | not_run | not_run |
| 5 | not_started | not_run | not_run | not_run |

## Current step work record

- Step: P0.1
- Intended files: none recorded yet
- Partial edits: none
- Required source reads still pending: see Phase 0
- Implementation blockers: none established
- Known risks: shader/interface parity, fingerprint/reset policy separation, transparent per-layer sequencing, abandoned recording chronology, freeze window off-by-one behavior.

## Completion log

Plan authored on 2026-10-22 after source inspection. Only eight plan Markdown files were created. Local Markdown links and sequential phase step IDs were checked successfully. No implementation or runtime result is claimed.

Append one record per completed step with this exact information:

```text
Step ID and date:
State: completed | implemented_unverified | blocked
Files and symbols changed:
Behavior/API decisions:
Static checks and results:
Build authorization and commands, if any:
Runtime backend/device/scenarios and actual results, or not_run:
Remaining limitations/failed checks:
Next step ID, phase file, and first concrete action:
```

## Resume consistency rule

Update the resume pointer, phase ledger, current work record, and completion log together after every step. Do not leave the pointer on a completed step. When changing a contract, update the phase document and fixed decisions too. Record user steering here before dependent implementation; never infer permission to build from permission to implement.
