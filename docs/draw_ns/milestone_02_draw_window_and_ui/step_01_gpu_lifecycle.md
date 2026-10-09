# Step 01: Track submissions and preserve frames in flight

## Objective and existing entry points
Keep Queue::submit asynchronous. Do not wait immediately after each normal frame submission. Wait only when a frame slot is reused while its previous work is still pending, for explicit immediateSubmit operations, or for global lifecycle operations.

The current Queue/CommandBuffer wrappers already know their owning Device. Metal has two native commit sites: Queue::submit and Device::immediateSubmit. Vulkan likewise has ordinary queue submission and a separate immediate submission path. Route both paths through the same internal per-device submission coordination without changing their public submit signatures. immediateSubmit remains intentionally blocking; it is not the normal frame path.

## Shared submission state
Introduce internal gpu::detail::SubmissionState, owned by Device and shared by its queue wrappers. It contains the admission gate, a native-queue identity/serialization mechanism, monotonic submission IDs, and pending CompletionRecords. A record is per submission, not per reusable CommandBuffer object. It owns the native completion lifetime and can query completion, wait explicitly and report terminal errors. Keep types independent of ui, draw and production render.

A CommandBuffer stores its owning device, recording/submission state and current completion record. Validate that it is submitted to its originating queue/device, reject submission while recording, duplicate Metal commits and reuse of a Vulkan command buffer still in flight. Queue::submit takes a raw pointer: do not require shared_from_this or keep only a raw wrapper pointer in the registry. The record retains required native objects independently; command/frame wrappers remain alive through slot ownership until completion.

Under the shared admission gate, validate, allocate/register the completion record and perform native submission atomically relative to waitIdle admission. Register before native submission can complete. On failed submission, remove or terminally fail the record and roll back slot bookkeeping; never leave a record waiting on an unsignaled completion object. Serialize access to a native queue even when graphics/present/transfer wrappers alias it. Registry access and command-pool allocation/reset/free synchronization are separate responsibilities and must both be covered.

Metal records retain the native MTL command buffer. Use its status() for nonblocking retirement and waitUntilCompleted() only for explicit waits. Treat a native error as terminal completion and surface the error rather than retaining it forever. The vendored Metal-cpp exposes these operations; no completion callback or Objective-C blocks implementation is required for this initial tracking design. Completed-handler integration can be added later, but must never capture a destructed Device or wrapper.

Vulkan window records may borrow a slot fence, provided completion is latched in the old record before the fence is reset for another submission. Non-frame sends need their own completion fence (or the immediate path's explicitly managed fence). A token must not mistake a recycled fence for its original submission. Release/reset command buffers only after completion; recycle or free them rather than accumulating allocations.

## Device::waitIdle admission contract
1. Serialize concurrent waitIdle callers and close the device-wide gate to new submits on every owned queue.
2. Ensure submissions already admitted have finished their native submit transaction and collect their records.
3. Release the registry mutex while waiting; keep admission closed. Recording on other threads may continue, but sending waits on the gate. Never hold a mutex that a completion/error path needs while waiting for GPU work.
4. Metal waits for all tracked outstanding records. Vulkan retains vkDeviceWaitIdle, with the same admission gate around it, then retires completed records.
5. Reopen admission and wake blocked submitters on success or recoverable error. Handle closing/device loss explicitly; do not resume submissions to a destroyed device.

This covers work sent through gpu wrappers, including immediateSubmit. Direct commits/submissions through public native handles bypass the tracker and admission gate; explicitly exclude those from the abstract contract unless separately registered. The engine's own code must not bypass it.

waitIdle does not leave submission permanently blocked after it returns. Resource mutation after its return needs application-level coordination that prevents new consumers from submitting against the old resources. For draw resize/shutdown, pause scene work and coordinate background GPU producers before draining. If this is insufficient for a future multi-producer API, add a scoped submission-pause guard rather than making waitIdle require an undocumented unlock. Do not call waitIdle while depending on a future producer submission that the gate itself blocks.

## Slot reuse instead of per-frame waits
Maintain a completion record and retained frame/command references per surface frame slot. On beginFrame, wait only for the record belonging to the slot being reused, then retire it before object-owned buffers/resource sets in that slot can be updated. Vulkan already waits its current slot fence and image-use fence here; preserve that behavior. Metal adds equivalent per-slot record waiting before acquiring a new drawable. Two slots permit two submitted frames concurrently.

WindowSurface::present already associates a Vulkan command with a frame. Extend Metal's equivalent association so Queue::submit installs its record into the current slot before endFrame advances. OffscreenSurface::present has no display operation but must perform this association as well. If a slot contains multiple submissions, retain all relevant records; a last submission only covers a prefix of the same properly ordered queue, never unrelated queues.

endFrame advances the logical slot/frame counter after submission, not after GPU completion. Keep acquired drawable/image wrappers alive in the slot until the corresponding work completes. Null or invalid acquisitions do not advance slots. Device waitIdle is not called during ordinary slot reuse.

## Deferred cleanup
Retain the public CleanupManager::defer syntax but replace frame-count-only readiness with captured pending submission records. Add internal Device access to the manager through Surface (which already stores its Device), without exposing native objects to callers. At defer time capture every still-pending device submission that may use retired resources; a conservative device-wide snapshot is acceptable. Later submissions must not reference the retired resource.

flushDeferred polls these immutable dependencies and executes a closure only when all are terminally completed; it does not block or assume frameCounter + inFlightFrames proves completion. At shutdown, coordinate producers, waitIdle and flushAllDeferred before destroying resources. Existing GPU examples retain their API calls and gain this safer completion-based readiness. Define removal/error handling for dependencies so cleanup remains bounded without hiding device errors.

## Acquisition and resize
Metal beginFrame checks a null drawable before accessing its texture and returns unavailable. Vulkan handles acquire/present failures explicitly and replaces recursive out-of-date retry with bounded retry/skip. Zero drawable size postpones acquisition/recreation. Add Surface::generation and increment it when targets are recreated/resized, so draw observes internal recreation. generation and frameCounter do not certify completion.

## End state
All engine-owned gpu submissions participate in tracking and waitIdle admission. Frames remain asynchronous, with waiting at slot reuse only. Native waitIdle implementation details remain confined to backend implementations. No render changes, tests or build invocation.
