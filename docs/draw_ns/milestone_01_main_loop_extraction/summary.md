# Milestone 01: Extract MainLoop graphics execution

## Objective
Preserve every existing production application entry point while extracting graphics-dependent work from `bg2e::app::MainLoop`. Introduce the smallest experimental `bg2e::draw` contracts needed to select a second execution path. This milestone completes the production extraction, not experimental rendering.

Existing `run(Application*)` selects production `render`; new `run(Application*, const draw::EngineConfig&)` selects experimental `draw`. The overload creates the internal execution implementation. Application authors do not construct or configure the wrapper.

## Non-negotiable constraints
- Do not modify any header or source under `bg2e::render`, including its delegates. Use production code only as a behavioral reference.
- Production application and example source code stays unchanged. `render` remains supported, maintained production API; introduce no deprecation attributes or messages.
- Leave platform macros where they are. Include `bg2e/base/PlatformTools.hpp` wherever `BG2E_IS_*` is needed.
- Do not implement GPU synchronization changes, native ImGui interoperability, or multibackend widgets in this milestone.
- Do not introduce a GPU-to-UI or GPU-to-draw dependency.
- Do not add tests. Compilation after each executed step belongs to the project lead; implementation agents must not invoke builds as part of these steps.
- Each numbered step must leave all introduced symbols defined and translation units structurally buildable. An unfinished runtime path throws explicitly rather than silently selecting production or pretending to initialize.

## Architecture
`MainLoop` owns the SDL window lifecycle, input, shortcuts, timers, preferences, scheduling, resize debounce, work queues, Loader and the common `UserInterface` object. An internal `app::detail::GraphicsExecution` owns the selected engine and rendering coordinator. It receives non-owning references to the application and UserInterface.

`RenderGraphicsExecution` encapsulates the current `render::Engine` and `render::RenderLoop` with their existing behavior, including descriptor initialization and Vulkan UI callback. `DrawGraphicsExecution` owns the experimental `draw::Engine` and `draw::RenderLoop` shell. GraphicsExecution is implementation infrastructure, not a new application API.

Keep the extraction interface free of native Vulkan and Metal types. It may use SDL_Window, Application, UserInterface, glm::vec4 and scalar frame parameters. Backend-specific includes belong to concrete implementation files.

## Resource direction for draw
Resources updated per in-flight frame belong to the object using them. Such an object owns persistent slot resources, reuses the synchronized current slot and releases its resources at object cleanup. There is no central per-frame descriptor allocator initialization callback in the new delegate. `gpu::FrameResourceRing<T>` may implement individual resource rings where T derives from DeviceResource; it is not a generic ring for arbitrary object types.

Engine owns the GPU context and global lifecycle. RenderLoop coordinates acquisition, delegate work, UI composition and presentation. These roles do not imply a one-to-one copy of render classes.

Future draw rendering retains an independent 3D image while UI can continue updating. The internal extraction must not require that every presentation redraw the scene. Production pause behavior remains unchanged.

## Ordered steps
1. [Extract the production implementation](step_01_extract_production_execution.md).
2. [Route MainLoop graphics operations](step_02_route_main_loop.md).
3. [Introduce draw contracts](step_03_define_draw_contracts.md).
4. [Add Application registration and run overload](step_04_add_draw_selection.md).
5. [Finish ownership and compatibility integration](step_05_finalize_integration.md).

## Completion state
Existing applications run through the extracted production implementation without source edits. The draw overload validates the selected delegate and configuration, then reports that experimental execution is not available yet, before SDL/GPU resources are created. All experimental shells have defined out-of-line methods. The next milestone replaces this deliberate runtime boundary with real initialization.

## Scope boundaries
OffscreenApplication, GPUSelectionDialog, scene, db, manipulation, render-dependent UI widgets and production render internals are unchanged. Binary ABI stability across library versions is not promised by this extraction; consumers rebuild against the updated library. Source compatibility of existing Application and MainLoop call sites is required.

## Completion notes (step 05 audit)
- Final internal names: `bg2e::app::detail::GraphicsExecution` (`lib/src/bg2e/app/detail/GraphicsExecution.hpp`), implemented by `RenderGraphicsExecution` (`RenderGraphicsExecution.cpp`) and `DrawGraphicsExecution` (`DrawGraphicsExecution.cpp`); factories `createRenderGraphicsExecution()` and `createDrawGraphicsExecution(const draw::EngineConfig&)`; milestone boundary operation `ensureRuntimeAvailable()`.
- No MainLoop subclass or external code accessed the removed `_engine`/`_renderLoop` members, so no production-only protected-member bridge was required. The only cross-boundary access was the UI viewport size, solved by the new public `ui::UserInterfaceDelegate::setInitialSize(width, height)` (only assigns when no previous size exists).
- Production partial-initialization guarantee: `RenderGraphicsExecution` tracks established stages (engine, UI, render loop) and cleans up only those stages. No claim is made that all render partial failures are recoverable; pre-existing render failure limitations remain outside the extraction.
- Execution destructors invoke the stage-guarded cleanup inside a try/catch so cleanup stays noexcept during exception unwinding and the original exception is preserved.
- `MainLoop::asyncLoad` and `executeSafeUpdateScene` address the active execution; `asyncLoad` throws `std::logic_error` outside an active run. Detached asyncLoad workers remain a known lifecycle limitation; backend replacement while a run or its workers are active is unsupported.
- Deferred topics: real `draw` GPU initialization, draw UI composition, the retained scene target and removal/no-op of `ensureRuntimeAvailable` belong to milestone 02. Experimental execution currently stops with `std::logic_error("Experimental draw execution requires milestone 02")` before any SDL/GPU resource is created.
