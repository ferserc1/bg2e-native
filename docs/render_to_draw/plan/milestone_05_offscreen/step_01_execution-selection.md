# M05-01 — Public offscreen contracts and production/draw init overloads

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** Accepted M04-08 cumulative documentation closure after M04-07 review; MainLoop execution pattern.

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


## M04-08 acceptance and M05-01 delivery

The lead's request to implement the next step accepts M04-08 under the execution
protocol; no backend/scenario details were specified. M05-01 adds
`OffscreenExecutionMode`, the native-free `DrawOffscreenApplicationConfig` and
`DrawOffscreenApplicationDelegate`, plus a typed `OffscreenApplication::init`
overload. The production delegate overload remains source-compatible and now
rejects null delegates/reinitialization. Draw extent is validated before selecting
the experimental mode. Invalid/mixed use cannot reach production Vulkan delegate
callbacks through this overload.

The delegate exposes config, engine/surface initialization, scene initialization,
resize, fixed update, a borrowed draw::FrameContext recording callback, completed
scene-frame callback, continuation and cleanup. New headers contain no Vk types or
render:: includes. M05-02 owns actual Engine/OffscreenSurface initialization,
submission and callback execution. Until then the draw mode's run path reports that
missing execution implementation explicitly; no windowless rendering is claimed
by M05-01. The internal execution coordinator remains a private M05-02 choice; the
public mode enum records which typed init overload selected.

## Project lead checks for this delivery

Compile the library and existing production offscreen examples to check source
compatibility for the existing delegate overload. A small caller may derive from
the new draw delegate and select the draw overload on Vulkan and, where available,
Metal; verify the config callback sees defaults and rejects a zero width or height
before any engine allocation. The draw run intentionally stops with the M05-02
handoff message in this intermediate step. Do not count that as a rendering test.
