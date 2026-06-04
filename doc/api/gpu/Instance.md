# Instance

**Header:** `<bg2e/gpu/Instance.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API Instance {
public:
    virtual ~Instance() = default;

    virtual void enableDebugMode(bool value) = 0;
    [[nodiscard]] virtual bool debugModeEnabled() const = 0;
    virtual void setApplicationName(const std::string& name) = 0;
    [[nodiscard]] virtual const std::string& applicationName() const = 0;

    virtual void create(SDL_Window* window) = 0;
    virtual void create() = 0;
    virtual void cleanup() = 0;

    [[nodiscard]] PresentationMode presentationMode() const;
    [[nodiscard]] SDL_Window* window() const;

protected:
    PresentationMode _presentationMode = PresentationMode::Undefined;
    SDL_Window* _window = nullptr;
};
```

Represents a GPU API instance (Vulkan instance or Metal device system). Must
be created before any other GPU object. Manages debug mode, application
naming, and creation with or without a window.

---

## Methods

### `virtual void enableDebugMode(bool value) = 0`

Enables or disables debug/validation layers. Must be called before `create()`.

| Parameter | Type   | Description                                    |
|-----------|--------|------------------------------------------------|
| `value`   | `bool` | `true` to enable validation, `false` to disable. |

### `virtual bool debugModeEnabled() const = 0`

Returns `true` if debug mode is currently enabled.

### `virtual void setApplicationName(const std::string& name) = 0`

Sets the application name reported to the GPU driver. Must be called before
`create()`.

| Parameter | Type                 | Description        |
|-----------|----------------------|--------------------|
| `name`    | `const std::string&` | Application name.  |

### `virtual const std::string& applicationName() const = 0`

Returns the current application name.

### `virtual void create(SDL_Window* window) = 0`

Creates the instance in windowed mode, attaching it to the given SDL window.

| Parameter | Type          | Description               |
|-----------|---------------|---------------------------|
| `window`  | `SDL_Window*` | The SDL window to attach. |

Sets `presentationMode()` to `PresentationMode::Windowed`.

### `virtual void create() = 0`

Creates the instance in headless/offscreen mode (no window). Sets
`presentationMode()` to `PresentationMode::Offscreen`.

### `virtual void cleanup() = 0`

Destroys the instance and releases all associated resources.

### `PresentationMode presentationMode() const`

Returns the presentation mode. Only meaningful after `create()` has been called.

### `SDL_Window* window() const`

Returns the SDL window pointer, or `nullptr` if the instance was created in
offscreen mode.

---

## vk::Instance

**Header:** `<bg2e/gpu/vk/Instance.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::Instance`

```cpp
class Instance : public gpu::Instance {
public:
    Instance();

    void setApplicationName(const std::string& name) override;
    const std::string& applicationName() const override;
    void enableDebugMode(bool value) override;
    bool debugModeEnabled() const override;

    void create(SDL_Window* window) override;
    void create() override;
    void cleanup() override;

    VkInstance vkInstanceHnd() const;

    bool getRequiredLayers(std::vector<const char*>& requiredLayers) const;
    bool getRequiredExtensions(SDL_Window* window,
                               std::vector<const char*>& requiredExtensions) const;
    bool getRequiredExtensions(std::vector<const char*>& requiredExtensions) const;
};
```

Vulkan instance wrapper. Manages `VkInstance`, debug messenger, validation
layers, and extensions.

### Vulkan-specific methods

#### `VkInstance vkInstanceHnd() const`

Returns the raw `VkInstance` handle. Useful for passing to Vulkan extension
functions or third-party libraries.

#### `bool getRequiredLayers(std::vector<const char*>& requiredLayers) const`

Fills `requiredLayers` with the names of validation layers to enable. Returns
`true` if all requested layers are available.

| Parameter        | Type                        | Description          |
|------------------|-----------------------------|----------------------|
| `requiredLayers` | `std::vector<const char*>&` | Output: layer names. |

#### `bool getRequiredExtensions(SDL_Window* window, std::vector<const char*>& requiredExtensions) const`

Fills `requiredExtensions` with the names of Vulkan extensions required for
windowed rendering (includes surface and platform-specific extensions).

| Parameter            | Type                        | Description              |
|----------------------|-----------------------------|--------------------------|
| `window`             | `SDL_Window*`               | The SDL window.          |
| `requiredExtensions` | `std::vector<const char*>&` | Output: extension names. |

#### `bool getRequiredExtensions(std::vector<const char*>& requiredExtensions) const`

Fills `requiredExtensions` with the names of Vulkan extensions required for
headless rendering (no surface extensions).

| Parameter            | Type                        | Description              |
|----------------------|-----------------------------|--------------------------|
| `requiredExtensions` | `std::vector<const char*>&` | Output: extension names. |

---

## metal::Instance

**Header:** `<bg2e/gpu/metal/Instance.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::Instance`

```cpp
class Instance : public gpu::Instance {
public:
    Instance();

    void setApplicationName(const std::string& name) override;
    const std::string& applicationName() const override;
    void enableDebugMode(bool value) override;
    bool debugModeEnabled() const override;

    void create(SDL_Window* window) override;
    void create() override;
    void cleanup() override;
};
```

Metal instance. On macOS, validates Metal support. Does not use SDL for
surface creation (Metal uses its own layer system).

Does not provide methods or properties beyond the base `gpu::Instance`
interface.
