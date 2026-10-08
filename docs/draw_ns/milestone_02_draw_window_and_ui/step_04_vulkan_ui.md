# Step 04: Add backend-specific ImGui execution and Vulkan integration

## UserInterface storage
Move private GPU backend storage into PImpl. Preserve the production init(render::Engine*) and draw(VkCommandBuffer, VkImageView) entry points with equivalent behavior. Keep legacy Vulkan declarations as required for source compatibility; introduce no Metal declarations in public UI headers. Existing protected-member usage must be audited before moving fields; do not rewrite production application source to compensate.

Add init(draw::Engine*), init(draw::Engine*, UserInterface*) to UserInterfaceDelegate as the new virtual initialization overload (the latter belongs to the delegate, not UserInterface), and draw(gpu::CommandBuffer&, gpu::SurfaceFrame&). Declare/define these together. Include a virtual delegate destructor for safe use through base ownership. Keep drawUI and logical viewport dimensions unchanged. Unsupported Metal initialization still throws explicitly until step 05.

Internally define UI backend operations for initialize, prepareFrame, draw and shutdown. Keep SDL/context/style/font/event/frameOverride behavior common. Define a prepared-frame entry point taking the acquired frame/command context; newFrame() without arguments remains the production entry point. The new draw coordinator calls preparation only after acquisition and calls composition on the same presentation target.

## Native pass materialization
Vulkan beginRendering defers native vkCmdBeginRendering. Add a Vulkan-specific public materializeRenderPass operation that invokes pending-pass emission and rejects calls without an active rendering scope. Do not place ImGui types in its signature. handle() alone does not materialize a pass.

For the UI overlay, beginRendering(presentationColorImage) without depth, without clear; materialize the native scope; call ImGui_ImplVulkan_RenderDrawData on the native command buffer; endRendering. The loop owns surrounding transitions. UI does not submit or present.

## Initialization
Get checked Vulkan concrete instances from draw's abstract Engine objects: vkInstanceHnd, device/physical-device handles, graphics queue handle and family. Own the descriptor pool inside UI. Use dynamic rendering, actual presentation color format and one sample. Keep pointer-backed init structures alive as required by the vendored backend rather than depending on stack data beyond the documented use.

Use surface.imageCount for ImageCount; derive a valid minimum count from Vulkan surface capabilities, respecting the vendored ImGui minimum of two. Do not use inFlightFrames as image count. On generation changes, refresh pipeline/count configuration where needed after GPU completion.

## Lifetime
Track context, SDL and renderer initialization independently. Shutdown the selected renderer, SDL backend and context before device/window destruction, only for initialized stages. For the production route retain existing engine-registered cleanup ordering so this extraction does not cause double ImGui shutdown or callbacks accessing destroyed state.

Reset static style/font bookkeeping when a context is destroyed so a future context does not inherit stale initialization flags. Guard processEvent/newFrame for an uninitialized context during staged draw execution.

## End state
Draw Vulkan displays UI over preserved scene color. Production public entry points remain compatible. No widget migration, tests or build invocation.
