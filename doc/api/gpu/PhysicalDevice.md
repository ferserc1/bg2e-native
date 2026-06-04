# PhysicalDevice

**Header:** `<bg2e/gpu/PhysicalDevice.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API PhysicalDevice {
public:
    virtual ~PhysicalDevice() = default;

    static void listSuitableDevices(
        const Instance* instance,
        const Surface* surface,
        std::vector<std::shared_ptr<PhysicalDeviceProperties>>& result
    );

    virtual void choose(Instance& instance, Surface& surface) = 0;
    virtual bool isValid() const = 0;
    virtual const std::shared_ptr<PhysicalDeviceProperties> properties() const = 0;
};
```

Represents a physical GPU device. Use `choose()` to automatically select the
best available device, or `listSuitableDevices()` to enumerate all candidates.

---

## Methods

### `static void listSuitableDevices(const Instance* instance, const Surface* surface, std::vector<std::shared_ptr<PhysicalDeviceProperties>>& result)`

Enumerates all suitable physical devices for the given instance and surface.
Appends `PhysicalDeviceProperties` objects to `result`, sorted by score
(highest first).

| Parameter  | Type                                  | Description                     |
|------------|---------------------------------------|---------------------------------|
| `instance` | `const Instance*`                     | The GPU instance.               |
| `surface`  | `const Surface*`                      | The rendering surface.          |
| `result`   | `std::vector<std::shared_ptr<...>>&`  | Output vector (cleared first).  |

### `virtual void choose(Instance& instance, Surface& surface) = 0`

Selects the best physical device for the given instance and surface. After
calling this, `properties()` and `isValid()` reflect the chosen device.

| Parameter  | Type         | Description            |
|------------|--------------|------------------------|
| `instance` | `Instance&`  | The GPU instance.      |
| `surface`  | `Surface&`   | The rendering surface. |

### `virtual bool isValid() const = 0`

Returns `true` if a device has been successfully chosen.

### `virtual const std::shared_ptr<PhysicalDeviceProperties> properties() const = 0`

Returns the properties of the chosen device. Only valid after `choose()` has
been called successfully.

---

## vk::PhysicalDevice

**Header:** `<bg2e/gpu/vk/PhysicalDevice.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::PhysicalDevice`

```cpp
class PhysicalDevice : public gpu::PhysicalDevice {
public:
    void choose(gpu::Instance& instance, gpu::Surface& surface) override;
    bool isValid() const override;
    const std::shared_ptr<PhysicalDeviceProperties> properties() const override;

    VkPhysicalDevice handle() const;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;
        std::optional<uint32_t> transfer;

        bool isComplete() const;
        bool isCompleteHeadless() const;
    };

    QueueFamilyIndices queueFamilyIndices() const;

    static const std::vector<const char*>& getRequiredDeviceExtensions(bool offscreen);
};
```

Vulkan physical device wrapper. Selects a suitable `VkPhysicalDevice`,
queries queue family indices, and reports properties including ray tracing
support.

### Vulkan-specific methods

#### `VkPhysicalDevice handle() const`

Returns the raw `VkPhysicalDevice` handle.

#### `QueueFamilyIndices queueFamilyIndices() const`

Returns the queue family indices for the chosen device. Only valid after
`choose()`.

#### `static const std::vector<const char*>& getRequiredDeviceExtensions(bool offscreen)`

Returns a reference to the list of required Vulkan device extensions.

| Parameter   | Type   | Description                                     |
|-------------|--------|-------------------------------------------------|
| `offscreen` | `bool` | `true` to exclude swapchain-related extensions. |

### Nested struct: QueueFamilyIndices

```cpp
struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    std::optional<uint32_t> transfer;

    bool isComplete() const;
    bool isCompleteHeadless() const;
};
```

| Field      | Type                      | Description                                    |
|------------|---------------------------|------------------------------------------------|
| `graphics` | `std::optional<uint32_t>` | Queue family index supporting graphics.        |
| `present`  | `std::optional<uint32_t>` | Queue family index supporting presentation.    |
| `transfer` | `std::optional<uint32_t>` | Queue family index supporting transfer ops.    |

#### `bool isComplete() const`

Returns `true` if `graphics`, `present`, and `transfer` all have values.

#### `bool isCompleteHeadless() const`

Returns `true` if `graphics` and `transfer` have values (ignores `present`).

---

## metal::PhysicalDevice

**Header:** `<bg2e/gpu/metal/PhysicalDevice.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::PhysicalDevice`

```cpp
class PhysicalDevice : public gpu::PhysicalDevice {
public:
    ~PhysicalDevice() override;

    void choose(gpu::Instance& instance, gpu::Surface& surface) override;
    bool isValid() const override;
    const std::shared_ptr<PhysicalDeviceProperties> properties() const override;

    DeviceHandle metalDevice() const;
};
```

Metal physical device wrapper. Wraps `MTL::Device*` via the `DeviceHandle`
typedef.

### Metal-specific methods

#### `DeviceHandle metalDevice() const`

Returns the raw `MTL::Device*` handle (on macOS) or an opaque pointer (on
other platforms).
