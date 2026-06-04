# vk::Info

**Header:** `<bg2e/gpu/vk/Info.hpp>`
**Namespace:** `bg2e::gpu::vk`

```cpp
class BG2E_API Info {
public:
    static VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo(
        PFN_vkDebugUtilsMessengerCallbackEXT callback,
        void* pUserData = nullptr);

    static VkCommandPoolCreateInfo commandPoolCreateInfo(
        uint32_t queueFamilyIndex,
        VkCommandPoolCreateFlags flags = 0);

    static VkCommandBufferAllocateInfo commandBufferAllocateInfo(
        VkCommandPool pool, uint32_t count = 1);

    static VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0);
    static VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);
    static VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);

    static VkSemaphoreSubmitInfo semaphoreSubmitInfo(
        VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

    static VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd);

    static VkSubmitInfo2 submitInfo(
        VkCommandBufferSubmitInfo* cmdInfo,
        VkSemaphoreSubmitInfo* signalSemaphoreInfo,
        VkSemaphoreSubmitInfo* waitSemaphoreInfo);

    static VkSubmitInfo2 submitInfo(
        VkCommandBuffer cmd,
        VkPipelineStageFlags2 waitSemaphoreStageFlags, VkSemaphore waitSemaphore,
        VkPipelineStageFlags2 signalSemaphoreStageFlags, VkSemaphore signalSemaphore);

    static VkPresentInfoKHR presentInfo(
        VkSwapchainKHR& swapchain, VkSemaphore& waitSemaphore, uint32_t& imageIndex);

    static VkImageCreateInfo imageCreateInfo(
        VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent,
        uint32_t arrayLayers = 1,
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);

    static VkImageViewCreateInfo imageViewCreateInfo(
        VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

    static VkRenderingAttachmentInfo attachmentInfo(
        VkImageView view, VkClearValue* clearValue,
        VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    static VkRenderingAttachmentInfo depthAttachmentInfo(
        VkImageView view, float depthValue = 1.0f,
        VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    static VkRenderingInfo renderingInfo(
        VkExtent2D renderExtent,
        VkRenderingAttachmentInfo* colorAttachment,
        VkRenderingAttachmentInfo* depthAttachment,
        uint32_t colorAttachmentsCount = 1);

    static VkPipelineLayoutCreateInfo pipelineLayoutInfo();
};
```

Static utility class providing factory methods for Vulkan struct initialization.
Every method returns a pre-initialized Vulkan create/info struct with sensible
defaults, reducing boilerplate throughout the engine.

---

## VK_ASSERT

**Header:** `<bg2e/gpu/vk/common.hpp>`

```cpp
#define VK_ASSERT(x)  /* ... */
```

Error-checking macro for Vulkan API calls. Evaluates the expression `x`, which
must return a `VkResult`. If the result is not `VK_SUCCESS` (i.e., non-zero),
it prints the error to `std::cerr` and throws `std::runtime_error` with a
human-readable error string obtained from `string_VkResult()`.

**Usage:**

```cpp
VK_ASSERT(vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer));
```

---

## Methods

### `static VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo(PFN_vkDebugUtilsMessengerCallbackEXT callback, void* pUserData = nullptr)`

Creates a debug messenger create info struct.

| Parameter   | Type                                    | Description                   |
|-------------|-----------------------------------------|-------------------------------|
| `callback`  | `PFN_vkDebugUtilsMessengerCallbackEXT`  | Debug callback function.      |
| `pUserData` | `void*`                                 | User data passed to callback. |

### `static VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0)`

Creates a command pool create info struct.

| Parameter          | Type                       | Description                        |
|--------------------|----------------------------|------------------------------------|
| `queueFamilyIndex` | `uint32_t`                 | Queue family for the command pool. |
| `flags`            | `VkCommandPoolCreateFlags` | Pool creation flags (default: 0).  |

### `static VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1)`

Creates a command buffer allocate info struct.

| Parameter | Type            | Description                               |
|-----------|-----------------|-------------------------------------------|
| `pool`    | `VkCommandPool` | Pool to allocate from.                    |
| `count`   | `uint32_t`      | Number of command buffers to allocate.    |

### `static VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0)`

Creates a fence create info struct.

### `static VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0)`

Creates a semaphore create info struct.

### `static VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0)`

Creates a command buffer begin info struct.

### `static VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore)`

Creates a semaphore submit info struct for Vulkan 1.3 synchronization.

| Parameter   | Type                    | Description                     |
|-------------|-------------------------|---------------------------------|
| `stageMask` | `VkPipelineStageFlags2` | Pipeline stages to wait/signal. |
| `semaphore` | `VkSemaphore`           | The semaphore.                  |

### `static VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd)`

Creates a command buffer submit info struct.

| Parameter | Type              | Description         |
|-----------|-------------------|---------------------|
| `cmd`     | `VkCommandBuffer` | The command buffer. |

### `static VkSubmitInfo2 submitInfo(...)`

Two overloads for creating `VkSubmitInfo2` structs (Vulkan 1.3
synchronization). The first takes pre-built info structs; the second takes
raw handles and builds the info structs internally.

### `static VkPresentInfoKHR presentInfo(VkSwapchainKHR& swapchain, VkSemaphore& waitSemaphore, uint32_t& imageIndex)`

Creates a present info struct for queue presentation.

| Parameter       | Type              | Description               |
|-----------------|-------------------|---------------------------|
| `swapchain`     | `VkSwapchainKHR&` | The swapchain to present. |
| `waitSemaphore` | `VkSemaphore&`    | Semaphore to wait on.     |
| `imageIndex`    | `uint32_t&`       | Swapchain image index.    |

### `static VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, uint32_t arrayLayers = 1, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT)`

Creates an image create info struct.

| Parameter     | Type                    | Description             |
|---------------|-------------------------|-------------------------|
| `format`      | `VkFormat`              | Image format.           |
| `usageFlags`  | `VkImageUsageFlags`     | Image usage flags.      |
| `extent`      | `VkExtent3D`            | Image dimensions.       |
| `arrayLayers` | `uint32_t`              | Number of array layers. |
| `samples`     | `VkSampleCountFlagBits` | MSAA sample count.      |

### `static VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags)`

Creates an image view create info struct.

| Parameter     | Type                 | Description                     |
|---------------|----------------------|---------------------------------|
| `format`      | `VkFormat`           | Image format.                   |
| `image`       | `VkImage`            | The image to create a view for. |
| `aspectFlags` | `VkImageAspectFlags` | Aspect mask (color/depth).      |

### `static VkRenderingAttachmentInfo attachmentInfo(VkImageView view, VkClearValue* clearValue, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)`

Creates a color rendering attachment info for dynamic rendering.

| Parameter    | Type            | Description                   |
|--------------|-----------------|-------------------------------|
| `view`       | `VkImageView`   | The image view.               |
| `clearValue` | `VkClearValue*` | Clear value (may be nullptr). |
| `layout`     | `VkImageLayout` | Expected image layout.        |

### `static VkRenderingAttachmentInfo depthAttachmentInfo(VkImageView view, float depthValue = 1.0f, VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)`

Creates a depth rendering attachment info for dynamic rendering.

| Parameter    | Type            | Description               |
|--------------|-----------------|---------------------------|
| `view`       | `VkImageView`   | The depth image view.     |
| `depthValue` | `float`         | Clear depth value.        |
| `layout`     | `VkImageLayout` | Expected image layout.    |

### `static VkRenderingInfo renderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment, uint32_t colorAttachmentsCount = 1)`

Creates a rendering info struct for dynamic rendering.

| Parameter               | Type                         | Description                     |
|-------------------------|------------------------------|---------------------------------|
| `renderExtent`          | `VkExtent2D`                 | Render area dimensions.         |
| `colorAttachment`       | `VkRenderingAttachmentInfo*` | Color attachment(s).            |
| `depthAttachment`       | `VkRenderingAttachmentInfo*` | Depth attachment (or nullptr).  |
| `colorAttachmentsCount` | `uint32_t`                   | Number of color attachments.    |

### `static VkPipelineLayoutCreateInfo pipelineLayoutInfo()`

Returns a default pipeline layout create info with no set layouts and no push
constants.
