# BakerContext and ownership

Header: `<bg2e/bake/BakerContext.hpp>`. Namespace: `bg2e::bake`.
BakerContext is the abstract bridge between UV jobs and scene/GPU resources.
BakerContextConfiguration borrows Device and retains the scene root, environment
and BakePassConfiguration owners. Injected UV surface, AO/GI, denoise, accumulator
and composition components belong exclusively to one bake lifecycle.

BakeFrame borrows a command buffer and identifies slot, global serial and source
generation. Slot reuse requires completion; all recorded resource owners survive
submission. One live target/context prevents mutable chart/history state leaking
between jobs. Stop producers and drain before destroying context/device. Display
passes and bake passes must have separate mutable state.

Settings and geometry changes invalidate generations. Freeze validated settings
for a job, restart after edits, preserve chart isolation and publish only completed
validated results. Structural configuration validation is not native capability
validation. Required unavailable effects fail the job.

Use [IntegratedBakerContext](IntegratedBakerContext.md) for interactive jobs or
[StandaloneBakerContext](StandaloneBakerContext.md) for camera-less batches.
See [settings](LightmapSettings.md), [UV surfaces](UvSurfacePass.md) and
[accumulation/output](LightmapOutput.md).
