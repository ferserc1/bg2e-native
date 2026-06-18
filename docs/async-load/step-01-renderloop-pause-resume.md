# Step 01 — RenderLoop: scene pause / resume

## Files

| File | Action |
|---|---|
| `lib/include/bg2e/render/RenderLoop.hpp` | modify |
| `lib/src/bg2e/render/RenderLoop.cpp` | modify |

## Changes

### `RenderLoop.hpp`

Add three public methods and two private members:

```cpp
// Public
void pauseScene(glm::vec4 clearColor = {0.f, 0.f, 0.f, 1.f});
void resumeScene();
inline bool isScenePaused() const { return _scenePaused; }

// Private
bool      _scenePaused = false;
glm::vec4 _sceneClearColor { 0.f, 0.f, 0.f, 1.f };
```

Add `#include <glm/glm.hpp>` if not already pulled in transitively.

### `RenderLoop.cpp` — `pauseScene()` / `resumeScene()`

```cpp
void RenderLoop::pauseScene(glm::vec4 clearColor)
{
    _engine->device().waitIdle();
    _sceneClearColor = clearColor;
    _scenePaused = true;
}

void RenderLoop::resumeScene()
{
    _engine->device().waitIdle();
    _scenePaused = false;
}
```

### `RenderLoop.cpp` — `acquireAndPresent()` modifications

Inside both the MSAA branch and the non-MSAA branch, replace the `render()`
call (and its surrounding layout transitions) with a conditional block:

```
if (_scenePaused)
{
    // transition image → TRANSFER_DST_OPTIMAL
    // vkCmdClearColorImage with _sceneClearColor
    // transition image → COLOR_ATTACHMENT_OPTIMAL
}
else
{
    // existing render() call + layout transitions (unchanged)
}
```

The UI callback, the final transition to `PRESENT_SRC_KHR`, and everything
after it remain **exactly as they are** — the UI always renders.

#### Detailed layout sequence when paused (MSAA branch)

```
msaaImage:   UNDEFINED → TRANSFER_DST_OPTIMAL
             vkCmdClearColorImage
             TRANSFER_DST_OPTIMAL → COLOR_ATTACHMENT_OPTIMAL

resolveImage: skip (not used when paused in MSAA path)
```

Because MSAA resolve is also skipped, `resolveImage` must still reach
`COLOR_ATTACHMENT_OPTIMAL` before the UI callback writes to it.
Transition `resolveImage`: `UNDEFINED → COLOR_ATTACHMENT_OPTIMAL` at the
start of the paused MSAA branch.

#### Detailed layout sequence when paused (non-MSAA branch)

```
resolveImage: UNDEFINED → TRANSFER_DST_OPTIMAL
              vkCmdClearColorImage
              TRANSFER_DST_OPTIMAL → COLOR_ATTACHMENT_OPTIMAL
```

### VkClearColorValue helper

```cpp
VkClearColorValue clearValue{};
clearValue.float32[0] = _sceneClearColor.r;
clearValue.float32[1] = _sceneClearColor.g;
clearValue.float32[2] = _sceneClearColor.b;
clearValue.float32[3] = _sceneClearColor.a;

VkImageSubresourceRange range{};
range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
range.baseMipLevel   = 0;
range.levelCount     = 1;
range.baseArrayLayer = 0;
range.layerCount     = 1;

vkCmdClearColorImage(
    cmd,
    targetImage->handle(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    &clearValue,
    1, &range
);
```

## Compile-time check

At this point no API is exposed yet to callers so no other translation units
change. The build must succeed with the same compiler warnings as before.

## Tests / manual verification

* Launch any existing example; verify it renders normally (pause flag is false
  by default).
* From a debugger or temporary test code, call `pauseScene()` and confirm:
  - The viewport clears to the chosen colour.
  - The ImGui overlay still renders.
  - Calling `resumeScene()` restores normal rendering.
