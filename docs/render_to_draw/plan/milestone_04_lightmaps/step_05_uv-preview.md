# M04-05 — Bake UV map preview and UI integration

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M04-01/M04-02/M04-04 and M02 widget decisions.

The [M02 widget review](../milestone_02_manipulation/review.md) establishes that
draw texture previews retain UI-session GPU snapshots and must be released before
UI/Engine teardown. Bake UV preview output needs its own lifecycle and texture
registration, and must retire/rebind after resolution or source reload; it cannot
reuse production widget handles or assume draw material preview lifetime covers
bake outputs.

## Scope and deliverable

- `lib/include/bg2e/render/UvMapPreviewRenderer.hpp`
- `lib/include/bg2e/ui/UvMapPreview.hpp`
- `lib/include/bg2e/bake/UvMapPreviewRenderer.hpp`
- `shaders/src/bake/uv_preview/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/UvMapPreviewRenderer.hpp](../../../../lib/include/bg2e/render/UvMapPreviewRenderer.hpp), [lib/include/bg2e/ui/UvMapPreview.hpp](../../../../lib/include/bg2e/ui/UvMapPreview.hpp).

## Implementation work

1. Reimplement public UvMapPreviewRenderer in bake for selected CPU UV sets, submesh fills/wireframes and atlas boundary rendering through GPU primitives.
2. Expose completed sampled output as a draw/GPU texture resource with clear ownership, synchronous convenience and resolution replacement semantics.
3. Adapt the UV widget using a draw/bake overload or a new name if structurally simpler; preserve production UI path until M09.
4. Keep mesh inspection nonmutating; no native Vulkan drawing or ImGui types in public preview APIs.

## Verification and exit conditions

- Preview UV0/UV1 before/after atlas generation and after resolution changes without stale UI texture registrations.
- The graphics preview service works without a main renderer and remains configurable/reusable by the application.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M06/M07 use this exact preview service; M08 may use it as a diagnostic, not as a batch requirement.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
