# Milestone 02: Experimental draw window, retained scene color and ImGui

## Objective
Make the draw run overload operational on Vulkan and Metal. Add `examples/draw/01_window_ui/` with the same Application/MainLoop entry style as production examples. The example clears a retained scene image, presents it and shows the bg2e::ui demo window. It requires no 3D pipelines or shaders.

## Constraints
Production render headers/sources, delegates and application source remain unchanged. No deprecation. No GPU dependency on ui/draw. Platform macros stay in PlatformTools.hpp. General draw/app/ui headers contain no Metal includes, Objective-C types or MTL names. Native backend interoperability stays in backend-specific implementation.

Each ordered step leaves declarations, definitions and platform guards consistent so the project can compile at that boundary. Runtime completeness is only required at the milestone end. No tests are added. The project lead performs compilation after each step; these implementation tasks contain no build invocations.

## Scene and UI separation
RenderLoop owns a retained scene color image with transfer-source and color-attachment capabilities. Presentation images support transfer destination and color attachment. First initialize the scene image by clearing it. On a dirty unpaused scene, update/render it. Every allowed presentation copies the retained scene color into the acquired image, draws UI on top and presents.

UI is drawn into the presentation image, never into the retained scene image. This is sufficient to preserve the last 3D result while UI updates. A separate persistent UI texture is not required for this minimum milestone; a future UI offscreen target can replace the direct overlay without changing scene ownership.

Scene pause suppresses scene updates/draws but keeps UI and presentation alive. CPU work on a worker can leave the main thread updating UI; blocking the main thread or mutating a scene concurrently is not supported by this design. The example's UI control demonstrates frozen scene content with live UI.

## Ordered steps
1. [Establish completion and safe acquisition](step_01_gpu_lifecycle.md).
2. [Initialize draw Engine](step_02_engine_initialization.md).
3. [Render retained scene color and present](step_03_scene_and_presentation.md).
4. [Expose native pass interoperability and integrate Vulkan UI](step_04_vulkan_ui.md).
5. [Integrate Metal UI](step_05_metal_ui.md).
6. [Add the example and finish application integration](step_06_example_and_completion.md).

## Build integration scope
The example requires registration in examples/CMakeLists.txt and an example-local CMakeLists.txt; vendored Metal ImGui needs APPLE-only source selection. These are explicit implementation requirements of this milestone, not permission to restructure unrelated build configuration. AGENTS.md restricts CMake edits without explicit user authorization; obtain that authorization when executing this milestone if it has not already been granted. The present task creates planning documents only.

## Deferred scope
No scene migration, PBR renderer, texture-widget migration, general resource editor adaptation, offscreen Application migration, multiple windows or simultaneous backend engines. Do not derive draw resource allocation policy from production's per-frame descriptor allocator. Advanced asynchronous multi-frame optimization can follow after the minimal safe lifecycle is complete.

## Completion
The example runs through MainLoop with either low-level backend, clears the scene target, overlays the UI demo, handles resize/minimize/restore, retains scene content during pause and shuts down with completed GPU work. Production remains supported through its unchanged API. Runtime acceptance belongs to the project lead.
