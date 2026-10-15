# Runtime renderer lifecycle (M01-05)

## Ownership and initialization

`draw::DefaultRenderLoopDelegate` is a non-template RenderLoopDelegate with exclusive `unique_ptr<Renderer>` ownership and a retained shared scene root. Construct it with a non-null renderer and optional RendererConfiguration, then install it through the existing Application draw delegate/MainLoop draw selection. No production execution-selection behavior changes. The class can be used directly with `setScene`, or subclassed to override `createScene`; the default creates an empty root. `createScene` runs once from initScene unless an explicit scene has been queued.

The first frame commits the renderer owner before acquisition; native initialization waits for the actual nonzero retained target extent and format. Configuration output extent/format are resolved from that target. A zero initial internal extent follows output; an explicit internal extent remains fixed across output resizes. Initial layer extents are resolved consistently before structural validation. This is a startup/resize rule, not a parallel live preferences model; after initialization, renderer.configuration is the live state. M01-13 can extend internal-resolution policy on the renderer rather than cache settings in the delegate.

Lifecycle: initialize engine delegate, create/retain scene, initialize renderer against resolved target, attachScene, bind targets, then update/record on dirty scene frames. Renderer update owns scene traversal/animation policy; the delegate does not run a second scene visitor. All native resource/descriptor lifetime stays with its users; no centralized native descriptor allocator callback exists.

## Queued changes and safe boundary

All delegate setters/accessors and loop lifecycle methods are main-thread operations. `setRenderer` and `setScene` only queue changes, including calls made inside UI preparation, update or record. The latest pending renderer wins; superseded never-initialized owners are destroyed without an engine cleanup call. Scene replacement is independent, including an explicitly null scene; renderer replacement retains the same root unless a scene change was also queued.

RenderLoop calls `prepareFrame` before drawable acquisition and before any UI/scene command recording. DefaultRenderLoopDelegate drains GPU users, snapshots pending changes, invalidates/detaches targets, cleans and destroys the old initialized renderer, initializes the replacement and attaches the retained scene, then binds new targets and requests a scene refresh. Requests queued by detach/attach callbacks are preserved for the next boundary. This runs while paused; dirty scene work remains pending until resume. No partially recorded commands are abandoned for a replacement.

Background/native submitters must be stopped/coordinated by their owners throughout lifecycle changes. `waitIdle` does not authorize racing another producer. The loop prevents recursive frame execution, replacing its initialized delegate, or loop cleanup inside a frame callback. `RenderLoopDelegate::prepareFrame` and `surfaceChanged` are lifecycle hooks for loop use, not methods to invoke manually during UI command recording.

Resize already drains all target users. The new `surfaceChanged(extent, format)` hook preserves ordinary delegates by forwarding to their existing resize override. The default renderer delegate detaches targets, resizes the renderer and rebinds them. A format change cleans and reinitializes the same concrete owner with its live configuration, preserving scene and generation invalidation. A minimized window never allocates zero-sized targets. Pending changes may initialize against the last valid extent before a subsequent resize resolves the new one.

## Settings and delayed UI actions

`setRendererChangeCallbacks(detach, attach)` supplies lifecycle seams for M01-12, without implementing preferences early. Install callbacks before the first target binding, for example during application/UI initialization. Detach occurs after GPU completion and before old resources are released; attach receives the initialized renderer and the current generation. Resize and scene replacement also rebind, since their component/property targets may change.

`renderer()` is borrowed and only valid until the next lifecycle boundary. For delayed actions, capture a weak delegate and its `rendererGeneration`, then call `withRenderer(expectedGeneration, operation)` after locking. A stale generation, detached target or cleaned delegate returns false without calling the operation. Generation invalidates before detach callbacks run. The operation is synchronous on the main thread; it must not retain its borrowed Renderer reference. Capturing raw renderer/layer pointers in queued UI actions bypasses this contract and remains invalid.

MainLoop::safeUpdateScene still drains GPU users and provides its own weak SafeUpdateToken cancellation. Neither a GPU wait nor that token validates an obsolete raw property target: use delegate generation checks as well. setRenderer invoked by a safe-update callback still commits at the next prepareFrame. UI settings catalogs must cancel/detach old property callbacks before teardown and rebind through the attach hook.

## Timing, dirty scene and cleanup

FrameContext appends fields, preserving existing aggregate initializers and the presentation-based frameNumber/deltaSeconds:

- `sceneFrameNumber`: zero-based next scene submission number. It advances only after a successful queue submission containing scene work; UI-only/paused/no-drawable iterations do not advance it. Histories must use this plus their own renderer/scene reset generation, not presentation count.
- `sceneDeltaSeconds`: accumulated active delta since the previous successful scene submission. Paused intervals are excluded and the accumulator resets on pause/scene submission. Ordinary deltaSeconds stays in presentation seconds for existing delegates.
- RenderLoop exposes renderedSceneCount and presentedFrameCount separately. They reset on cleanup; renderedSceneCount advances after successful submission, presentation count after endFrame.

A clean scene retains its image while presentation/UI continue. `setContinuousRendering(true)` explicitly invalidates from update for the next scene frame; disabling it permits the next outstanding dirty frame, then retention. Custom delegates can call protected requestSceneFrame during update/record. Changes queued during recording preserve the dirty revision. Paused presentation retains the last valid scene image and pending dirty state; resume requests a refresh.

RenderLoop drains GPU work before delegate cleanup and disconnects its invalidation callback even when cleanup throws. Default cleanup detaches targets, attempts renderer cleanup (also after partial initialization), releases queued/active renderer and scene ownership, clears callbacks and engine references, and rethrows the first cleanup error after releasing state. Direct users must synchronize before calling cleanup. Destruction alone is not a synchronization boundary. Initialization/binding failure releases partially initialized renderer resources and propagates the failure; there is no silent fallback renderer or promise to recover a failed frame. After recording/submission failure, shut down/recreate the execution as required by GPU record-time layout tracking.

## Verification and remaining work

The agent inspected signatures/callers, legacy resize forwarding, ownership, callback invalidation, exception paths, field initialization order, local links and whitespace. No build, shader compilation, executable/test or runtime operation was run. CMake files were not changed in this step.

The [manual lifecycle fixture](../../../../tests/draw/renderer_lifecycle/README.md) uses a minimal clear renderer because RendererDeferred does not exist until M01-10. The project lead should compile the library and ordinary draw examples, run this fixture on available Vulkan/Metal windows, then check real UI, minimize/restore, actual extent/format changes and background throttling. M01-15 must exercise replacement of actual RendererDeferred instances after M01-12 settings rebinding exists. Pending M03 histories and M05 headless execution must share these scene submission/lifecycle semantics.
