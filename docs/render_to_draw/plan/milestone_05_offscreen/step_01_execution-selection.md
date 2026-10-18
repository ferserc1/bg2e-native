# M05-01 — Public offscreen contracts and production/draw init overloads

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** Accepted M04 review; MainLoop execution pattern.

## Scope and deliverable

- `lib/include/bg2e/app/OffscreenApplication.hpp`
- `lib/src/bg2e/app/OffscreenApplication.cpp`
- `lib/include/bg2e/app/MainLoop.hpp`
- `lib/include/bg2e/app/OffscreenExecution.hpp`
- `lib/include/bg2e/app/DrawOffscreenApplicationDelegate.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/app/OffscreenApplication.hpp](../../../../lib/include/bg2e/app/OffscreenApplication.hpp), [lib/src/bg2e/app/OffscreenApplication.cpp](../../../../lib/src/bg2e/app/OffscreenApplication.cpp), [lib/include/bg2e/app/MainLoop.hpp](../../../../lib/include/bg2e/app/MainLoop.hpp).

## Implementation work

1. Extract execution-mode responsibilities analogous to MainLoop while retaining existing production init/delegate behavior.
2. Add an initialization overload with draw configuration and an experimental delegate contract using GPU/FrameContext types. Prefer separate typed delegate interfaces over requiring new users to override legacy pure Vulkan callbacks.
3. Keep new execution/configuration classes publicly available where practical, per engine policy; document any indispensable internal class instead of automatically hiding the seam.
4. Define explicit configuration/lifecycle/update/record/completed-frame/cleanup hooks and no MSAA. Window/input/UI are not prerequisites.

## Verification and exit conditions

- Calling each init overload selects only its intended engine implementation; invalid mixed delegates/configurations fail before resources are allocated.
- Current production CLI behavior remains intact under authorized checks, while new headers expose no native Vulkan implementation.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M05-02/M05-03 use these public types. M08 standalone contexts reuse device ownership, and M09 makes the draw path canonical without legacy overloads.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.


## M04-07 reviewed implementation requirement

Define completed-frame hooks and failure retirement for the new offscreen execution itself. M04's sample application uses Device::immediateSubmit followed by completeFrame; it does not add completion callbacks to the window RenderLoop and does not provide an offscreen application implementation.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).
