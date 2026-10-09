# CleanupManager

**Header:** `<bg2e/gpu/CleanupManager.hpp>`

**Namespace:** `bg2e::gpu`

Provides ordered DeviceResource cleanup and completion-based deferred closures.
It borrows a Surface; defer requires that surface to be backed by a Device.
All manager operations belong to the owning render thread and are not thread-safe.

## Ordered cleanup

`push(shared_ptr<DeviceResource>)` registers normal resources, cleaned in reverse
insertion order. `pushStatic` registers static resources, cleaned first in
insertion order. Const-reference/rvalue overloads ignore nullptr.
`flush()` moves queues out before callbacks, attempts all cleanups, then rethrows
the first error. `clear()` only drops manager references; `empty()` concerns the
normal/static lists, not deferred closures. No method implicitly calls waitIdle.

## Deferred cleanup

`defer(std::function<void()>&&)` snapshots pending completion records from the
surface's device, across all its tracked queues. The closure is eligible when
all captured records are terminal; no frameCounter or targetFrame threshold is
used. Stop new uses of the retired resource before taking the snapshot. Recorded
but unsubmitted work and future submissions are outside the snapshot.

`flushDeferred()` polls dependencies and executes ready closures without a GPU
wait. It removes ready entries before callbacks. Execution failures recorded by
completion objects are propagated after inspection; a throwing closure currently
interrupts the remaining local ready batch. `flushAllDeferred()` unconditionally
executes pending closures, attempting all and rethrowing first error; invoke it
after coordinated waitIdle. Capture shared ownership to extend resource lifetime.

```cpp
auto retired = std::move(resource);
cleanup.defer([retired = std::move(retired)] { retired->cleanup(); });
// No later submission may use retired.
```

The frame loop polls after asynchronous submission/endFrame; endFrame is not a
GPU completion wait. A resource can become ready without additional presented
frames, but a caller must still poll the manager to run its closure.

```cpp
// Producers are stopped and remain stopped through teardown.
device->waitIdle();
cleanup.flushAllDeferred();
cleanup.flush();
surface->cleanup();
device->cleanup();
instance->cleanup();
```

See [implementation and source excerpts](Submission_tracking_and_waitIdle.md#completion-based-deferred-cleanup),
[resource management](DeviceResource_and_resource_management.md) and
[Device](Device.md). Callbacks and stored resources must not outlive native Device
resources required by their cleanup operations.
