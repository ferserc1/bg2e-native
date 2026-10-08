# Step 05: Integrate the Metal ImGui backend

## Dependency and source layout
Vendor imgui_impl_metal.h/.mm from the same ImGui revision as the repository's core, not from an arbitrary installed copy. Record the origin/revision. Compile the backend only on APPLE. Use a ui-specific Objective-C++ implementation such as lib/src/bg2e/ui/ImGuiMetalBackend.mm; keep Objective-C syntax and framework includes confined to APPLE-only files.

Use the backend's supported C++ bindings if present in that pinned revision; otherwise bridge Metal-cpp objects to Objective-C inside this translation unit only. Define IMGUI_IMPL_METAL_CPP consistently for relevant backend/interface compilation if taking the C++ path. Do not add another metal-cpp implementation unit or duplicate framework implementation macros.

The existing lib source glob includes APPLE-only .mm under src, but third-party ImGui's source glob includes only .cpp. Explicitly include the vendored backend .mm in APPLE-only dependency source selection; prevent non-Apple includes and duplicate compilation. Honor the milestone summary's CMake authorization boundary when executing this step.

## GPU native interoperability
Add Metal-specific renderPassDescriptor and materializeRenderEncoder operations to its CommandBuffer. Return borrowed objects valid only within the active rendering scope. materializeRenderEncoder calls the existing lazy encoder creation logic and reports misuse. Neither operation knows about ImGui. Keep native signatures in backend-specific headers/implementation with platform guards; general engine headers remain free of Metal.

For UI, use a color-only pass loading/storing the presentation image. Prepare the compatible render pass descriptor before ImGui_ImplMetal_NewFrame, materialize the encoder before RenderDrawData, and call endRendering exactly once afterward. ImGui must not commit commands, present a drawable, release the encoder or end encoding independently of gpu::CommandBuffer.

## Common UI ordering
Initialize SDL with ImGui_ImplSDL2_InitForMetal and the renderer using the native device. During a draw frame: acquire the presentation target; prepare backend frame metadata; prepare SDL/ImGui frame; execute drawUI/frameOverride; render scene/copy as coordinated by RenderLoop; open the actual UI pass and issue draw data.

A preparation-only descriptor may be separate from the actual overlay descriptor, provided color format, sample count and depth/stencil configuration match. Do not keep an encoder open while the scene pass or copy executes. Recreate preparation metadata when surface configuration changes.

## Failure and shutdown
Check backend initialization results. Keep unavailable drawables away from UI rendering. Shutdown Metal renderer/SDL/context after GPU completion and before Engine cleanup. Scene pause must not suppress UI frame preparation.

## End state
The abstract UI entry points work for draw Vulkan and Metal; Linux/Windows translation units contain no Metal dependency. Generic widgets and demo are supported, but production texture/scene editors remain outside scope. No tests or build invocation.
