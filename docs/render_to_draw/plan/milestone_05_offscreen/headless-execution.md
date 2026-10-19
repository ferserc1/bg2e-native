# M05-02 windowless draw execution

[Step](step_02_draw-offscreen-context.md) · [Progress](../progress.md) ·
[Manual probe](../../../../tests/draw/offscreen/README.md)

Starting HEAD: `bc3ccb7e78c178742fc9b0c210df8f2d0ee4527d`. Earlier uncommitted
M04 documentation and M05-01 work are preserved. M05-01 is accepted by project
lead continuation instruction; backend/scenario coverage is not specified.
This delivery completes the actual draw path previously selected by its typed
init overload. No CMake, production render, GPU implementation or shader changes.
The repository references skills.md, but no such file exists in this checkout;
AGENTS.md and the current architecture/plan are the inspected guidance.

## Context and ownership

`draw::Engine::init(gpu::Size2D, gpu::Backend&, EngineConfig const&)` creates an
Instance through create(), never create(window), and creates an OffscreenSurface.
Both init paths share physical/logical device, surface target and cleanup setup.
The factory Backend is retained, a caller-owned Backend must outlive Engine, and
the Backend owns the shared Instance wrapper. An active Instance is rejected before
creation and is not cleaned by a rejecting Engine. Cleanup retires submissions,
registered/deferred resources and surface owners before Device and Instance.

`surface()` remains WindowSurface* and returns null headlessly; renderSurface()
returns the generic Surface*, offscreenSurface() returns the typed headless
surface, and frameSlotCount() validates the generic surface's inFlightFrames.
RendererDeferred, DataBinding, DeferredLayer and query shadows now use generic
slots. DefaultRenderLoopDelegate resize reads the generic format. Window/UI callers
retain their window accessor; RenderLoop and UserInterface reject headless Engine
initialization explicitly. Existing GPU OffscreenSurface implementations already
provide the necessary target/submission tracking and need no new native code.

Normalized/floating color formats and nonzero extents are required. Depth can be
Undefined for direct Engine initialization; app createDepthImage=false selects
that value. Device formats/limits/allocation remain authoritative. The surface
image supports sampled color attachment and transfer access. RendererDeferred
still owns its own depth independently of an optional surface depth target.

## Application chronology

The typed OffscreenApplication init validates config, applies the appId fallback,
acquires the Backend, initializes Engine and calls delegate init, initScene,
resize and stopped prepareFrame. The partial-init marker is set before entering
the user init callback so failures also invoke cleanup. No SDL video/window,
MainLoop singleton, input or UI object is initialized by this path.

Before every requested frame the driver checks continueRendering, runs stopped
prepareFrame/update and checks continuation again. It acquires the completed slot,
clears its color, supplies the ordinary borrowed FrameContext and records through
the delegate. Active scopes left behind are errors. Headless present only binds
the command to its SurfaceFrame; the target never transitions to Present layout.
Device::immediateSubmit ends/submits and waits for completion. Only a successful
return allows endFrame, counter advancement and didRenderFrame. That callback
sees TransferSrc color and can read pixels while Engine/surface remain valid.
Auxiliary readback/environment/picking submissions do not advance scene counters.

Execution is serial, one slot on both current native OffscreenSurfaces. The first
frame uses index zero and delta zero; subsequent frames use the finite nonnegative
config.timeStepSeconds (default zero). Both FrameContext counters carry this
scene sequence. There is no wall-clock animation drift. Counters restart only with
a fresh initialization; run cleans the context and cannot be repeated uninitialized.

Failure after acquisition/recording/submission drains Device before delegate
cleanup and Engine teardown; no failed sample receives a completed callback.
Initialization/run failures preserve their first exception even if cleanup fails.
Cleanup errors on successful runs propagate. Destruction also retires initialized
but unrun draw contexts and suppresses destructor exceptions. Reentrant init/run
is rejected. Applications stop background producers during lifecycle boundaries;
custom cleanup must release its retained GPU owners while Device is still alive.

## Public renderer delegate

`draw::DefaultOffscreenApplicationDelegate` bridges the existing public
DefaultRenderLoopDelegate and typed app offscreen interfaces. It exclusively owns
one polymorphic Renderer through the existing delegate, retains the same scene
and reuses renderer replacement, generation, attach/detach, safe updates and
resolution reconfiguration. No duplicate renderer or scene traversal family is
introduced. createScene remains overridable. Initialization through either base
interface requires an actual offscreen Engine, preventing accidental window-loop
use of this bridge. Renderer::update(FrameContext) runs
once immediately before Renderer::record; the app update(seconds,index) callback
is a stopped simulation hook and does not traverse the scene again.

Constructor/setRenderer configurations remove overlays. prepareFrame also rejects
an incompatible custom renderer that publishes overlays. Default continuation
requests one frame; its private recorded flag is a stopping policy, not a public
completed-sample count. The driver consults it after confirmed completion. A
subclass overrides continuation to request more frames independently of dirty
window state or continuous-rendering flags. Completed-output policy belongs to the
application callback. No new private execution class or renderer template is added.

## Verification and remaining work

Agent evidence is static: public declarations/definitions, interface overloads,
raw readback/callback signatures, shared Instance/native surface and submission
tracking, required resource slots, lifecycle error ordering, complete owned GPL
headers, C/C++ braces and function separation, Markdown links and diff whitespace.
Required blank separators in touched DataBinding/DefaultRenderLoopDelegate/
RenderLoop implementations were corrected without changing tokens at those sites.

The manual probe adds known-value GPU completion/readback, zero frames, exception
preservation/cleanup, shared-Instance rejection and optional real deferred empty
scene coverage. It has not been compiled or executed. Vulkan/Metal/no-RT/SDK and
windowed/production regressions remain project-lead-owned; the matrix records
expected checks rather than observed results. No compiler, configure, fixture,
application/GPU, shader tool or packaging execution by the agent.

M05-03 must deliver exact linear camera-image averaging, active headless settings
catalog/import/precedence and final processing of the average once. This step
records ordinary per-frame renderer output, with its existing per-frame final
processing; N completions alone are not an accumulated image. M05-04/05 supply
the runnable scene CLI and encoded output. M08 may initialize Engine headlessly
and use public bake passes without instantiating a camera renderer; this does not
implement StandaloneBakerContext, scene assembly or batch output.

## M05-03 successor

The above M05-02 baseline is extended by [exact scene averaging and settings](accumulation-settings.md).
DefaultOffscreenApplicationDelegate now uses config.frames instead of a continuation
override, imports the live settings at initial prepareFrame during init, freezes
job settings, and requires the public optional Renderer linear/final seam.
Generic custom app delegates retain independent continuation. Only the last
completed callback with hasResult true carries the exportable averaged result;
final conversion is a separate completed helper submission, not another sample.
