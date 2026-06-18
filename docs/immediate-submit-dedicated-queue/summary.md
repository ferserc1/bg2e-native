# Dedicated Queue for `immediateSubmit`

## Problem Statement

`Command::immediateSubmit()` currently shares the same Vulkan graphics queue (`_graphicsQueue`) with the main render loop. When the async loading mechanism (`asyncLoad`) spawns a background thread that triggers operations using `immediateSubmit` (texture uploads, mesh creation, image transitions, etc.), two threads submit command buffers to the same queue without synchronization. This is undefined behavior in Vulkan and causes validation errors, potential deadlocks, and GPU hangs.

The core issue is that `vkQueueSubmit` on the same queue from two threads concurrently is a data race. The fence-based synchronous wait in `immediateSubmit` does not protect the queue submission itself.

## Proposed Solution

Create a second Vulkan graphics queue at device creation time, dedicated exclusively to `immediateSubmit` operations. Both queues belong to the same queue family (graphics), so command pools and command buffers remain compatible. The dedicated queue isolates background-thread submissions from the main render loop.

```
                        ┌─────────────────────────────────────┐
                        │          VkDevice                    │
                        │                                      │
  Main Render Thread    │  ┌──────────────────────────────┐   │
  ─────────────────────>│  │  Graphics Queue (index 0)     │   │
                        │  │  _graphicsQueue                │   │
                        │  │  Used by: RenderLoop,          │   │
                        │  │           FrameResources,      │   │
                        │  │           Swapchain present    │   │
                        │  └──────────────────────────────┘   │
                        │                                      │
  Background Thread     │  ┌──────────────────────────────┐   │
  (asyncLoad)     ─────>│  │  Immediate Queue (index 1)    │   │
                        │  │  _immediateQueue               │   │
                        │  │  Used by: Command::            │   │
                        │  │    immediateSubmit()           │   │
                        │  └──────────────────────────────┘   │
                        │                                      │
                        │  Queue Family: graphicsFamily        │
                        │  (both queues share same family)     │
                        └─────────────────────────────────────┘
```

**Key insight**: Command pools are associated with a queue *family*, not a specific queue. A command buffer allocated from a pool created on `graphicsFamily` can be submitted to *any* queue in that family. So the existing `_immediateCmdPool` (created on `graphicsFamily`) works for both queues without changes.

## Files to Modify

| File | Action | Description |
|------|--------|-------------|
| `lib/include/bg2e/render/vulkan/Device.hpp` | Modify | Add `_immediateQueue` member and `immediateQueue()` getter |
| `lib/src/bg2e/render/vulkan/Device.cpp` | Modify | Request 2 graphics queues, retrieve queue at index 1 |
| `lib/src/bg2e/render/vulkan/Command.cpp` | Modify | Store and use `_immediateQueue` in `init()` and `immediateSubmit()` |
| `lib/include/bg2e/render/vulkan/Command.hpp` | Modify | Add `_immediateQueue` protected member |

## Thread Safety Notes

- `immediateSubmit` is already synchronous: it submits, then blocks on `vkWaitForFences`. This means only one `immediateSubmit` call can be in-flight at a time on the dedicated queue, which is the correct behavior.
- The main render loop's `vkQueueSubmit` calls on `_graphicsQueue` are completely decoupled from submissions on `_immediateQueue`. No mutex is needed.
- The `_immediateCmdFence` is per-submit (reset before, wait after), so no shared state between threads.
- `createCommandPool()` continues to use `_graphicsQueueFamily`. Command pools on the same family work with any queue in that family.

## Step Files

- [Step 01: Device Queue Changes](step-01-device-queue-changes.md) — Add second graphics queue to `Device`
- [Step 02: Command Queue Changes](step-02-command-queue-changes.md) — Use dedicated queue in `Command::immediateSubmit`
