# Queue

**Header:** `<bg2e/gpu/Queue.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API Queue {
public:
    virtual ~Queue() = default;
    virtual uint32_t familyIndex() const = 0;
    virtual bool isValid() const = 0;

    virtual std::shared_ptr<gpu::CommandBuffer> createCommandBuffer(const std::string& debugName = {}) const = 0;
    virtual void submit(gpu::CommandBuffer* cmd) const = 0;
};
```

Abstract command queue. Concrete implementations are provided by `vk::Queue`
and `metal::Queue`.

---

## Methods

### `virtual uint32_t familyIndex() const = 0`

Returns the queue family index. On Metal, this always returns 0 (Metal has no
queue family concept).

### `virtual bool isValid() const = 0`

Returns `true` if the queue has been successfully created and is ready to
accept commands.

### `virtual std::shared_ptr<gpu::CommandBuffer> createCommandBuffer(const std::string& debugName = {}) const = 0`

Creates a command buffer, optionally labeling it with debugName. Vulkan uses a
reusable exclusive pool per live allocation; Metal creates a one-shot native
command buffer from its queue.
The returned command buffer must be recorded and submitted through the same
queue.

### `virtual void submit(gpu::CommandBuffer* cmd) const = 0`

Submits a recorded command buffer to the queue for execution. The command
buffer must have been created by this queue and fully recorded (i.e., `end()`
must have been called).

| Parameter | Type               | Description                    |
|-----------|--------------------|--------------------------------|
| `cmd`     | `gpu::CommandBuffer*` | The command buffer to submit.|

---

## vk::Queue

**Header:** `<bg2e/gpu/vk/Queue.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::Queue`

```cpp
class Queue : public gpu::Queue {
public:
    Queue() = default;
    Queue(VkQueue queue, uint32_t family);

    uint32_t familyIndex() const override;
    bool isValid() const override;

    std::shared_ptr<gpu::CommandBuffer> createCommandBuffer(const std::string& debugName = {}) const override;
    void submit(gpu::CommandBuffer* cmd) const override;

    VkQueue handle() const;
};
```

Vulkan command queue wrapper. Stores the `VkQueue` handle and its family
index. Allocates command buffers from an internal `VkCommandPool`.

### Constructor

#### `Queue(VkQueue queue, uint32_t family)`

| Parameter | Type       | Description              |
|-----------|------------|--------------------------|
| `queue`   | `VkQueue`  | The Vulkan queue handle. |
| `family`  | `uint32_t` | The queue family index.  |

### Vulkan-specific methods

#### `VkQueue handle() const`

Returns the raw `VkQueue` handle.

---

## metal::Queue

**Header:** `<bg2e/gpu/metal/Queue.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::Queue`

```cpp
class Queue : public gpu::Queue {
public:
    Queue() = default;
    explicit Queue(CommandQueueHandle commandQueue);
    ~Queue() override;

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    Queue(Queue&&) noexcept;
    Queue& operator=(Queue&&) noexcept;

    uint32_t familyIndex() const override;
    bool isValid() const override;

    std::shared_ptr<gpu::CommandBuffer> createCommandBuffer(const std::string& debugName = {}) const override;
    void submit(gpu::CommandBuffer* cmd) const override;

    CommandQueueHandle handle() const;
};
```

Metal command queue wrapper. Wraps `MTL::CommandQueue*` via the
`CommandQueueHandle` typedef. Supports move semantics but not copy.

### Constructor

#### `explicit Queue(CommandQueueHandle commandQueue)`

| Parameter        | Type                 | Description              |
|------------------|----------------------|--------------------------|
| `commandQueue`   | `CommandQueueHandle` | The Metal command queue. |

### Metal-specific methods

#### `CommandQueueHandle handle() const`

Returns the raw `MTL::CommandQueue*` handle.

#### `uint32_t familyIndex() const override`

Always returns 0. Metal has no queue family concept.

## Submission lifecycle

submit validates owning device/queue and closed recording state, registers an
immutable completion record, then performs native submission under the shared
Device admission lock. Metal commit and Vulkan queueSubmit2 are asynchronous.
Vulkan presentation shares that transaction. Submitted Metal wrappers cannot be
recorded/submitted again; Vulkan begin requires the previous completion to be
terminal before resetting. Command recording itself is not made thread-safe.

All device queue wrappers share tracking, even if Vulkan wrappers alias a native
queue. Records retain native Metal buffers or Vulkan command allocations/fences
independently of the user wrapper. Acquisition waits previous slot records before
reuse. Full details: [submission tracking and waitIdle](Submission_tracking_and_waitIdle.md).
