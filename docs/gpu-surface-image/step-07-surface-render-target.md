# Step 7 — Surface render-target phase + `Device::create` hook

**Goal:** Make `gpu::Surface` the render-target provider. Add the render-target virtuals to the
base, implement them in all four concrete surfaces, hook `Device::create()` to trigger creation,
extend the `Backend` factory with format parameters, and update the examples.

This is the one larger step: introducing pure virtuals on the `Surface` base forces all four
concrete surfaces to implement them in the same step so the build stays green (and the
pure-abstract interface style is preserved).

## Files

- **Modify** `lib/include/bg2e/gpu/Surface.hpp`, `WindowSurface.hpp`, `OffscreenSurface.hpp`, `Backend.hpp`
- **Modify** `lib/include/bg2e/gpu/vk/WindowSurface.hpp` + `.cpp`, `vk/OffscreenSurface.hpp` + `.cpp`, `vk/Backend.hpp` + `.cpp`, `vk/Device.cpp`
- **Modify** `lib/include/bg2e/gpu/metal/WindowSurface.hpp` + `.cpp`, `metal/OffscreenSurface.hpp` + `.cpp`, `metal/Backend.hpp` + `.cpp`, `metal/Device.cpp`
- **Modify** `examples/gpu/02_device/src/main.cpp`, `examples/gpu/03_offscreen_device/src/main.cpp`

---

## 1. `gpu/Surface.hpp` (abstract base)

Add forward decls (`class Device; class PhysicalDevice; class Image;`), color/depth format
state + getters, the render-target virtuals, and the image accessors.

```cpp
#include <bg2e/gpu/Common.hpp>   // Size2D, PixelFormat

namespace bg2e { namespace gpu {
class Device; class PhysicalDevice; class Image;

class BG2E_API Surface {
public:
    virtual ~Surface() = default;

    virtual bool isOffscreen() const = 0;
    virtual bool isValid()     const = 0;

    const Size2D& size() const { return _size; }
    void setSize(const Size2D& s) { _size = s; }
    virtual uint32_t width()  const { return _size.width;  }
    virtual uint32_t height() const { return _size.height; }

    PixelFormat colorFormat() const { return _colorFormat; }
    PixelFormat depthFormat() const { return _depthFormat; }
    void setColorFormat(PixelFormat f) { _colorFormat = f; }   // set by Backend factory
    void setDepthFormat(PixelFormat f) { _depthFormat = f; }

    // Render-target phase. createRenderTarget() is invoked by Device::create();
    // resize()/releaseRenderTarget() are driven by higher-level code.
    virtual void createRenderTarget(Device* device, PhysicalDevice* physicalDevice) = 0;
    virtual void resize(const Size2D& size) = 0;
    virtual void releaseRenderTarget() = 0;

    virtual uint32_t    imageCount() const = 0;             // 1 offscreen, 2+ buffered windows
    virtual gpu::Image* colorImage(uint32_t index) const = 0; // may be nullptr (metal windowed)
    virtual gpu::Image* depthImage() const = 0;             // nullptr if depthFormat == Undefined

protected:
    Size2D          _size;
    PixelFormat     _colorFormat = PixelFormat::Undefined;
    PixelFormat     _depthFormat = PixelFormat::Undefined;
    Device*         _device         = nullptr;  // not owned; set in createRenderTarget
    PhysicalDevice* _physicalDevice = nullptr;  // not owned
};
}}
```

## 2. `gpu/WindowSurface.hpp` / `gpu/OffscreenSurface.hpp`

- `WindowSurface`: keep `create(Instance*)` / `cleanup()`. Document that `cleanup()` must also
  call `releaseRenderTarget()` (resources depend on the device, so release before device
  destruction).
- `OffscreenSurface`: keep `explicit OffscreenSurface(const Size2D&)` (from Step 3); add a
  `virtual void cleanup()` that calls `releaseRenderTarget()` for symmetry with `WindowSurface`.

## 3. `gpu/Backend.hpp` + concrete backends

Add format parameters (preferred color, depth) with sensible defaults:

```cpp
virtual std::unique_ptr<WindowSurface> createWindowSurface(
    PixelFormat colorFormat = PixelFormat::B8G8R8A8_UNORM,
    PixelFormat depthFormat = PixelFormat::D32_SFLOAT) const = 0;

virtual std::unique_ptr<OffscreenSurface> createOffscreenSurface(
    const Size2D& size,
    PixelFormat colorFormat = PixelFormat::R8G8B8A8_UNORM,
    PixelFormat depthFormat = PixelFormat::D32_SFLOAT) const = 0;
```

`vk::Backend` / `metal::Backend` overrides construct the concrete surface, then call
`setColorFormat(colorFormat)` / `setDepthFormat(depthFormat)` before returning it.

---

## 4. `Device::create()` hook (both backends)

At the **end** of `vk::Device::create` and `metal::Device::create`, after the device (and, for
Vulkan, the VMA allocator from Step 4) is fully initialized:

```cpp
if (surface) {
    surface->createRenderTarget(this, physicalDevice);
}
```

`this` is the just-initialized `gpu::Device*`; `physicalDevice` is the existing parameter. The
surface stores both (`_device`, `_physicalDevice`) for later `resize`/`releaseRenderTarget`.
Include `<bg2e/gpu/Surface.hpp>` (already included in `vk/Device.cpp`).

> Lifetime rule (documented + applied in the examples): the surface's render target depends on
> the device (swapchain/VMA images / Metal device-bound textures), so it must be released
> **before** `Device::cleanup()`. `Device::cleanup()` does not touch the surface.

---

## 5. `vk::WindowSurface`

Header members:
```cpp
VkSwapchainKHR                         _swapchain{VK_NULL_HANDLE};
std::vector<std::unique_ptr<vk::Image>> _colorImages;   // swapchain images wrapped
std::unique_ptr<vk::Image>             _depthImage;
// existing: _surface (VkSurfaceKHR), _window, _vkInstance
```
Add accessors: `VkSwapchainKHR swapchain() const;`

`createRenderTarget(device, physicalDevice)`:
1. `_device = device; _physicalDevice = physicalDevice;` cast to `vk::Device` / `vk::PhysicalDevice`.
2. Query `vkGetPhysicalDeviceSurfaceCapabilitiesKHR`, `...SurfaceFormatsKHR`, `...SurfacePresentModesKHR`
   using `vkPhys->handle()` + `_surface`.
3. Pick surface format: prefer `toVkFormat(_colorFormat)` with `SRGB_NONLINEAR` color space; else
   first available. Set `_colorFormat = fromVkFormat(chosen.format)` (report the actual format).
4. Pick present mode (`VK_PRESENT_MODE_FIFO_KHR` always available); compute extent (use caps
   `currentExtent`, else clamp `_size` to min/max). Update `_size`.
5. `imageCount = caps.minImageCount + 1`, clamped to `maxImageCount` (if non-zero).
6. Fill `VkSwapchainCreateInfoKHR` (usage `COLOR_ATTACHMENT | TRANSFER_DST`, `oldSwapchain` = a
   member kept across resize or `VK_NULL_HANDLE`) → `vkCreateSwapchainKHR`.
7. `vkGetSwapchainImagesKHR` → for each, `auto img = std::make_unique<vk::Image>();
   img->initFromSwapchainImage(vkDevice, vkImage, _colorFormat, _size); _colorImages.push_back(...)`.
8. If `_depthFormat != Undefined`: `_depthImage = std::make_unique<vk::Image>();
   _depthImage->buildDepthImage(vkDevice, _size, _depthFormat);`

`resize(size)`: `setSize(size)`; `releaseRenderTarget()`; `createRenderTarget(_device, _physicalDevice)`.
(Caller is responsible for `device->waitIdle()` first.) Optionally pass `oldSwapchain` for a
smoother recreate — optimization, not required now.

`releaseRenderTarget()`: reset `_depthImage`; clear `_colorImages` (their dtors destroy the
views; swapchain images themselves are not owned); `vkDestroySwapchainKHR(vkDevice->handle(),
_swapchain, nullptr); _swapchain = VK_NULL_HANDLE;`. Guard on `_device`.

`cleanup()` (existing): call `releaseRenderTarget()` first, then destroy `VkSurfaceKHR` as today.

`imageCount()` → `_colorImages.size()`. `colorImage(i)` → `_colorImages[i].get()`.
`depthImage()` → `_depthImage.get()`. `isValid()` → `_swapchain != VK_NULL_HANDLE`.

## 6. `vk::OffscreenSurface`

Header members:
```cpp
std::unique_ptr<vk::Image> _colorImage;
std::unique_ptr<vk::Image> _depthImage;
```
`createRenderTarget(device, physicalDevice)`: store device/physDev; cast device→`vk::Device`;
`_colorImage = make_unique<vk::Image>(); _colorImage->buildTargetImage(vkDevice, _size, _colorFormat);`
build `_depthImage` if `_depthFormat != Undefined`.

`resize(size)`: `setSize(size)`; if images exist `_colorImage->resize(size)` (+ depth), else build.
`releaseRenderTarget()`: reset both images. `cleanup()` → `releaseRenderTarget()`.
`imageCount()` → `1`. `colorImage(0)` → `_colorImage.get()` (else nullptr). `depthImage()` →
`_depthImage.get()`. `isValid()` → `_colorImage && _colorImage->isValid()`.

## 7. `metal::WindowSurface`

Header members (mac): keep `_metalView`, `_layer`; add
```cpp
std::unique_ptr<metal::Image> _depthImage;
uint32_t _imageCount = 0;
```
`createRenderTarget(device, physicalDevice)` (mac): store device/physDev; cast device→`metal::Device`.
Configure the existing `_layer`:
```cpp
_layer->setDevice(metalDevice->handle());
_layer->setPixelFormat(toMetalPixelFormat(_colorFormat));
_layer->setDrawableSize(CGSize{ double(_size.width), double(_size.height) });
_layer->setMaximumDrawableCount(3);     // triple-buffered by default
_imageCount = 3;
```
Build depth texture if `_depthFormat != Undefined`:
`_depthImage = std::make_unique<metal::Image>(); _depthImage->buildDepthImage(metalDevice, _size, _depthFormat);`

> The per-frame color comes from `CAMetalDrawable` (transient), so there is **no** persistent
> color `Image`. `colorImage(index)` returns `nullptr`; the color access path is the existing
> `metalLayer()` getter (presentation, out of scope here).

`resize(size)`: `setSize(size)`; `_layer->setDrawableSize(...)`; rebuild `_depthImage` (resize).
`releaseRenderTarget()`: reset `_depthImage` (the layer/view live until `cleanup()`).
`cleanup()` (existing): call `releaseRenderTarget()` first, then destroy the metal view as today.
`imageCount()` → `_imageCount`. `colorImage(_)` → `nullptr`. `depthImage()` → `_depthImage.get()`.
Non-mac: stubs as per existing convention.

## 8. `metal::OffscreenSurface`

Header members (mac): `std::unique_ptr<metal::Image> _colorImage, _depthImage;`
`createRenderTarget`: build color (`buildTargetImage`) + depth (`buildDepthImage`) textures.
`resize`: resize both. `releaseRenderTarget`: reset both. `cleanup()` → `releaseRenderTarget()`.
`imageCount()` → `1`. `colorImage(0)` → `_colorImage.get()`. `depthImage()` → `_depthImage.get()`.
`isValid()` → `_colorImage && _colorImage->isValid()`. Non-mac: stubs.

---

## 9. Example updates

### `examples/gpu/02_device/src/main.cpp`
- Surface creation unchanged (`backend->createWindowSurface(); surface->create(instance);`).
- After `device->create(...)`, the swapchain now exists — optionally print:
  `std::cout << "  Swapchain images: " << surface->imageCount() << std::endl;`
- **Cleanup order** (swapchain depends on device): release the surface render target before the
  device:
  ```cpp
  device->waitIdle();
  surface->cleanup();      // releases render target + destroys VkSurfaceKHR
  device->cleanup();
  instance->cleanup();
  SDL_DestroyWindow(window);
  SDL_Quit();
  ```

### `examples/gpu/03_offscreen_device/src/main.cpp`
- `auto surface = backend->createOffscreenSurface(gpu::Size2D{ 800, 600 });` (from Step 3).
- After `device->create(...)`, offscreen color+depth images now exist; optionally print
  `surface->imageCount()` / `surface->width()`.
- **Cleanup**: add the surface release before the device:
  ```cpp
  device->waitIdle();
  surface->cleanup();      // releases color/depth images
  device->cleanup();
  instance->cleanup();
  ```

---

## Compile check

- Base `Surface` pure virtuals are introduced together with all four concrete implementations.
- `Device::create` calls a virtual that every concrete surface implements.
- Metal code paths are `#if BG2E_IS_MAC`-guarded; non-mac builds use the stub surfaces.
- Examples updated for the new offscreen signature and cleanup ordering.

Build + both examples stay green. After this step the surface fully owns and reports the render
target (size, format(s), image count, color/depth images), with creation/resize/release only —
no presentation logic, as scoped.
