# Factory

**Header:** `<bg2e/gpu/Factory.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API Factory {
public:
    static void init(BackendType type);
    static Backend* backend();

private:
    static std::unique_ptr<Backend> _backend;
};
```

Entry point for the GPU abstraction layer. The Factory initializes and provides
access to the active GPU backend singleton. Call `Factory::init()` once at
startup, then use `Factory::backend()` to retrieve the `Backend*` for creating
all GPU subsystem objects.

---

## Methods

### `static void init(BackendType type)`

Creates and stores the backend singleton. Must be called once before any other
GPU operations.

| Parameter | Type          | Description                              |
|-----------|---------------|------------------------------------------|
| `type`    | `BackendType` | Which backend to create (`Vulkan` or `Metal`). |

**Throws:** `std::runtime_error` if the requested backend is not available on
the current platform.

### `static Backend* backend()`

Returns a pointer to the active backend. The returned pointer is owned by the
Factory and must not be deleted by the caller.

**Throws:** `std::runtime_error` if `init()` has not been called.

---

## Example

```cpp
#include <bg2e/gpu/all.hpp>

// Initialize Vulkan backend
gpu::Factory::init(gpu::BackendType::Vulkan);
auto* backend = gpu::Factory::backend();

// Create GPU objects through the backend
auto* instance = backend->instance();
auto surface = backend->createWindowSurface();
auto physicalDevice = backend->createPhysicalDevice();
auto device = backend->createDevice();
```
