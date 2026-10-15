# M01-05 — Non-template delegate and runtime renderer lifecycle

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M01-01.

## Scope and deliverable

- `lib/include/bg2e/draw/RenderLoopDelegate.hpp`
- `lib/include/bg2e/draw/RenderLoop.hpp`
- `lib/src/bg2e/draw/RenderLoop.cpp`
- `lib/include/bg2e/draw/DefaultRenderLoopDelegate.hpp`
- `lib/include/bg2e/app/MainLoop.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/draw/RenderLoopDelegate.hpp](../../../../lib/include/bg2e/draw/RenderLoopDelegate.hpp), [lib/include/bg2e/draw/RenderLoop.hpp](../../../../lib/include/bg2e/draw/RenderLoop.hpp), [lib/src/bg2e/draw/RenderLoop.cpp](../../../../lib/src/bg2e/draw/RenderLoop.cpp), [lib/include/bg2e/app/MainLoop.hpp](../../../../lib/include/bg2e/app/MainLoop.hpp).

## Implementation work

1. Implement a non-template default scene-renderer delegate holding a polymorphic Renderer owner. Retain scene across setRenderer/replacement and keep Application/MainLoop execution selection intact.
2. Define initialize, scene attach, extent change, update/record and cleanup contracts with no centralized native descriptor allocator callback.
3. Queue renderer replacement at a safe boundary: stop new work, wait for users, detach settings, release old extent/pass resources, initialize the replacement, rebind settings and request scene rendering.
4. Distinguish presentation count from rendered-scene sequence; retain pause/dirty-scene/UI composition behavior and explicit animation redraw requests.

## Verification and exit conditions

- The manual lifecycle fixture replaces minimal Renderer instances and resumes the same retained scene with invalidated/rebound target generations. RendererDeferred is not available until M01-10; the actual RendererDeferred-to-RendererDeferred example with the M01-12 settings facade is required in M01-15.
- Actual ownership, lifecycle hooks, scene counters and lead checks are documented in [runtime-renderer.md](runtime-renderer.md).
- No use-after-free on pending UI callbacks/commands; pause/resize/resume and ordinary draw examples remain functional.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M01-12 owns settings rebinding; M03 histories and M05 headless frames must share these rendered-scene/lifetime semantics.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
