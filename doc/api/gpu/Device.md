# Device

**Header:** `<bg2e/gpu/Device.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API Device {
public:
    virtual ~Device() = default;

    virtual void create(Instance* instance, PhysicalDevice* physicalDevice,
                        Surface* surface) = 0;
    virtual void cleanup() = 0;
    virtual void waitIdle() = 0;

    virtual bool isValid() const = 0;

    virtual const Queue& graphicsQueue() const = 0;
    virtual const Queue& presentQueue() const = 0;
    virtual const Queue& transferQueue() const = 0;
};
```

Represents a logical device created from a physical device. Provides access
to command queues.

---

## Methods

### `virtual void create(Instance* instance, PhysicalDevice* physicalDevice, Surface* surface) = 0`

Creates the logical device. The instance, physical device, and surface must
already be initialized.

| Parameter        | Type              | Description                 |
|------------------|-------------------|-----------------------------|
| `instance`       | `Instance*`       | The GPU instance.           |
| `physicalDevice` | `PhysicalDevice*` | The chosen physical device. |
| `surface`        | `Surface*`        | The rendering surface.      |

### `virtual void cleanup() = 0`

Destroys the logical device and releases all associated resources.

### `virtual void waitIdle() = 0`

Blocks until all pending GPU operations on this device have completed.

### `virtual bool isValid() const = 0`

Returns `true` if the device has been successfully created.

### `virtual const Queue& graphicsQueue() const = 0`

Returns the graphics command queue. Valid only after `create()`.

### `virtual const Queue& presentQueue() const = 0`

Returns the presentation command queue. Valid only after `create()`.

### `virtual const Queue& transferQueue() const = 0`

Returns the transfer command queue. Valid only after `create()`.

---

## vk::Device

**Header:** `<bg2e/gpu/vk/Device.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::Device`

```cpp
class Device : public gpu::Device {
public:
    void create(gpu::Instance* instance, gpu::PhysicalDevice* physicalDevice,
                gpu::Surface* surface) override;
    void cleanup() override;
    void waitIdle() override;

    bool isValid() const override;

    const gpu::Queue& graphicsQueue() const override;
    const gpu::Queue& presentQueue() const override;
    const gpu::Queue& transferQueue() const override;

    VkDevice handle() const;
};
```

Vulkan logical device wrapper. Creates `VkDevice` with the required queue
families and exposes the three queue types.

### Vulkan-specific methods

#### `VkDevice handle() const`

Returns the raw `VkDevice` handle.

---

## metal::Device

**Header:** `<bg2e/gpu/metal/Device.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::Device`

```cpp
class Device : public gpu::Device {
public:
    void create(gpu::Instance* instance, gpu::PhysicalDevice* physicalDevice,
                gpu::Surface* surface) override;
    void cleanup() override;
    void waitIdle() override;

    bool isValid() const override;

    const gpu::Queue& graphicsQueue() const override;
    const gpu::Queue& presentQueue() const override;
    const gpu::Queue& transferQueue() const override;

    DeviceHandle handle() const;
};
```

Metal logical device. Wraps the Metal device handle and exposes
graphics/present/transfer queues.

### Metal-specific methods

#### `DeviceHandle handle() const`

Returns the raw `MTL::Device*` handle.
