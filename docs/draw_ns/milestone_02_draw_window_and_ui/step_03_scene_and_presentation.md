# Step 03: Implement retained scene color and presentation coordination

## Targets
RenderLoop creates one retained gpu::Image matching the surface's actual color format and drawable pixel extent. Include color attachment and transfer source usage. If scene target initialization or resizing needs transfer destination usage, include it explicitly. Confirm both surface implementations create presentation images suitable for transfer destination and UI color attachment.

The scene image is a persistent result, not an acquired drawable. Keep it valid between presentations. Engine does not acquire ownership of it. Do not use SurfaceFrame as the scene delegate's permanent target.

## Frame sequence
1. Skip a zero-sized or unavailable presentation frame without advancing application-owned resource slots.
2. Acquire and validate SurfaceFrame. Compare surface generation/extent/format; synchronize and recreate the scene target when changed, then notify the delegate with drawable pixel size.
3. Select the synchronized frameSlot and frameNumber from Surface. Create or safely reuse the command wrapper. Convert incoming milliseconds to seconds.
4. Begin commands. If the scene target has never been initialized, clear it to configured/default background even when paused.
5. If unpaused and scene-dirty, update the delegate, transition the scene image to ColorAttachment, clear as required and render the scene. Require the delegate to close any scope it opens. This minimum delegate receives only the retained scene color target; depth support can be added later.
6. Transition scene to TransferSrc and presentation color to TransferDst; copy between matching format and extent targets through gpu::CommandBuffer::copyImage.
7. Transition presentation color to ColorAttachment. Leave a specific optional UI composition callback slot, invoked outside active scene rendering. It is empty until steps 04/05. Do not initialize UserInterface or invoke ImGui functions before its backend exists.
8. Transition presentation to Present; call surface.present, cmd.end, queue.submit, cmd.waitUntilCompleted, surface.endFrame and cleanupManager.flushDeferred, in that order.

Scene rendering starts dirty. Expose requestSceneFrame for invalidation; this example does not require unconditional scene redraw. pauseScene preserves the last valid image; resume marks dirty. Clear-color changes mark dirty. Existing pauseScene's color parameter is stored as a background for future scene refresh, not used to erase a valid retained image merely because a pause occurred.

## Execution wiring
Implement DrawGraphicsExecution initialize/initializeScene/frame/resize/pause/resume/waitIdle/cleanup against Engine and RenderLoop. Install the draw delegate. Remove the runtime boundary now that clear/presentation can execute. Keep experimental UI initialization absent until its implementation is available; processEvent/frameOverride operations are guarded against an uninitialized UserInterface. Production retains its existing UI behavior.

requestResize records invalidation. Actual resize uses drawable pixels after a positive size becomes available; UI viewport dimensions remain logical window units. Read drawable dimensions using backend-appropriate SDL calls inside app/backend implementation. Drain work before replacing targets and keep generation observations consistent with internally recreated Vulkan surfaces.

## End state
Draw can present cleared retained color without UI. Scene and UI updates are structurally independent. No resource-set initialization phase, tests or build invocation.
