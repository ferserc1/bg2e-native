# Draw API Reference

Public contracts introduced in milestone 01. The umbrella header is
`<bg2e/draw/all.hpp>`.

| Symbol | Header | Purpose and current availability |
|--------|--------|----------------------------------|
| [EngineConfig](EngineConfig.md) | `EngineConfig.hpp` | Aggregate backend/debug/name/format configuration; platform-aware backend default. |
| [Engine](Engine.md) | `Engine.hpp` | PImpl context shell; initialization/accessors currently throw. |
| [FrameContext](FrameContext.md) | `FrameContext.hpp` | Borrowed scene command/target references and frame metadata. |
| [RenderLoopDelegate](RenderLoopDelegate.md) | `RenderLoopDelegate.hpp` | Abstract GPU-based scene callbacks. |
| [RenderLoop](RenderLoop.md) | `RenderLoop.hpp` | Coordination state implemented; scene/frame execution pending. |
| Umbrella include | `all.hpp` | Includes all public draw contracts without backend-specific headers. |

## Related application API

- [MainLoop run overloads](../app/MainLoop.md#execution-selection-and-validation)
  select production render or experimental draw.
- [Application graphics registration](../app/Application_and_input.md#graphics-delegate-registration)
  stores separate render/draw delegate slots.
- [GPU API](../gpu/index.md) provides abstract backend resources.

A valid draw MainLoop configuration currently throws the milestone 02
availability exception before SDL/window/GPU allocation. Native backend
interop and draw UI initialization are not part of this milestone's public API.
