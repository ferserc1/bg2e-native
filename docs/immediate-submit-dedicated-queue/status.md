# Plan Status

## Step 01 completed: Device Queue Changes
Date: 2026-06-18
Changes:
- lib/include/bg2e/render/vulkan/Device.hpp: Added `_immediateQueue` member and `immediateQueue()` getter
- lib/src/bg2e/render/vulkan/Device.cpp: Request 2 queues for graphics family, retrieve queue at index 1 as `_immediateQueue`

## Step 02 completed: Command Queue Changes
Date: 2026-06-18
Changes:
- lib/include/bg2e/render/vulkan/Command.hpp: Added `_immediateQueue` protected member
- lib/src/bg2e/render/vulkan/Command.cpp: Retrieve immediate queue in `init()`, use `_immediateQueue` instead of `_graphicsQueue` in `immediateSubmit()`
