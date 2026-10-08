# Step 01: Establish a minimal safe GPU frame lifecycle

## Problem
Metal Device::waitIdle is currently empty; Queue::submit commits without waiting; Surface::endFrame increments a counter but does not establish GPU completion. Vulkan window submission also remains asynchronous. Counting submitted frames is insufficient for safe deferred destruction or reuse of object-owned slot resources.

## Minimal policy
Use one completed GPU frame at a time for this milestone. Add a backend-neutral CommandBuffer::waitUntilCompleted() virtual operation with a default implementation that throws for unsupported implementations; implement it for Vulkan and Metal in this same step. The new draw loop calls it after submit and before endFrame or releasing per-frame wrappers. This intentionally trades concurrency for a clear safe baseline.

Vulkan command submission must supply a completion fence even when no presentation frame is attached. Track whether the command owns the fence or borrows a SurfaceFrame fence, and destroy owned fences only after completion. Existing window submissions preserve surface semaphore/presentation behavior. Avoid double resetting or destroying borrowed fences. Check existing immediateSubmit paths before changing submission ownership.

Metal waits on the committed native command buffer. Define behavior for calls before submission as an explicit error. Make Device::waitIdle actually drain submissions tracked by its queues, including transfer work; retain tracked native references until completed and remove completed entries to prevent unbounded growth. The existing public meaning of waitIdle must match its name before draw cleanup relies on it.

Command wrapper disposal/reuse is safe only after completion. Resolve Vulkan command-buffer release/reset ownership rather than allocating indefinitely from a pool. Do not add UI symbols to GPU code.

## Acquisition and resize
Metal beginFrame handles null drawable before accessing texture, returning an unavailable frame. Vulkan handles acquire/present result codes explicitly, uses a bounded retry/skip policy instead of recursive retry, and never reads an undefined image index. Zero-sized surfaces skip acquisition and defer recreation until a positive drawable size is known.

Introduce Surface::generation() with a monotonically increasing value when presentation targets are recreated or resized. Maintain it in both window backends; the base can provide the accessor and counter. Draw compares generation after acquisition to notice internal recreations. Generation does not indicate GPU completion.

## Deferred cleanup contract
Document that endFrame is not intrinsically a completion fence. In the new serialized draw loop, completion precedes endFrame and flushDeferred. Draining all deferred resources at shutdown occurs after actual waitIdle. Existing GPU examples that flush by counters must either adopt the completion call at the same lifecycle point or be documented as needing synchronized slots; do not leave a stated completion guarantee contradicted by code.

## End state
Native completion and recoverable acquisition contracts exist for both backends. Draw is still behind its milestone 01 runtime boundary. No render changes, tests or build invocation.
