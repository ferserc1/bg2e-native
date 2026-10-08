# Application API Reference

Class and function catalog for `bg2e::app`. The umbrella header is
`<bg2e/app/all.hpp>`.

## Lifecycle

| Symbol | Header | Description |
|--------|--------|-------------|
| [`Application`](Application_and_input.md#application) | `Application.hpp` | Stores render, input, and user-interface delegates. |
| [`MainLoop`](MainLoop.md) | `MainLoop.hpp` | Interactive lifecycle, event processing, frame scheduling, timers, and safe work. |
| [`WindowConfig`](MainLoop.md#windowconfig) | `MainLoop.hpp` | Initial window position, size, state, flags, and size persistence. |
| `SafeUpdateToken` | `MainLoop.hpp` | Lifetime guard for queued safe updates. |

### Execution selection

| Member | Description |
|--------|-------------|
| `MainLoop::run(Application*)` | Production render/Vulkan execution. |
| `MainLoop::run(Application*, const draw::EngineConfig&)` | Experimental draw selection; explicit configuration argument, Metal default on macOS and Vulkan elsewhere; runtime pending milestone 02. |
| `Application::setRenderDelegate(shared_ptr<render::RenderLoopDelegate>)` | Registers production delegate and clears draw slot. |
| `Application::setRenderDelegate(shared_ptr<draw::RenderLoopDelegate>)` | Registers draw delegate and clears production slot. |
| `Application::setRenderDelegate(nullptr_t)` | Clears both graphics slots. |
| `Application::renderDelegate()` / `drawDelegate()` | Mutable references to the separate graphics delegate slots. |

See [validation and runtime availability](MainLoop.md#execution-selection-and-validation).
The internal graphics execution interface is not an application API.
[draw API reference](../draw/reference.md) documents the new contracts.
### MainLoop scheduling

| Member | Description |
|--------|-------------|
| `setBackgroundFrameRateLimitEnabled(bool)` | Enables/disables limiting while unfocused. Foreground remains unrestricted. |
| `backgroundFrameRateLimitEnabled()` | Returns the current runtime state. |
| `setBackgroundMaxFrameRate(double)` | Sets a positive finite maximum, including fractional FPS. |
| `backgroundMaxFrameRate()` | Returns the configured background rate. |
| `requestFrame()` | Thread-safe request to bypass one background deadline. |
| `safeUpdateScene(fn, token)` | Queues main-thread work and requests a prompt frame. |
| `timeout()` | Returns the loop-owned timer scheduler. |
| `requestResizeEvent()` | Requests resize handling and a frame. |
| `asyncLoad(fn, clearColor, completion)` | Runs loading work off-thread and completes safely on the main thread. |

## Scene and mesh updates

| Symbol | Header | Description |
|--------|--------|-------------|
| `Uv2RegenerationResult`, `Uv2SafeReload` | `Uv2SafeReload.hpp` | Queues CPU UV2 generation and a loaded Drawable GPU reload through `MainLoop::safeUpdateScene`; see [Uv2SafeReload](Uv2SafeReload.md). |

## Input

| Symbol | Header | Description |
|--------|--------|-------------|
| [`InputDelegate`](Application_and_input.md#inputdelegate) | `InputDelegate.hpp` | Virtual keyboard, mouse, wheel, and file-drop callbacks. |
| `InputManager` | `InputManager.hpp` | Delegate dispatcher and normalized cursor helper. |
| `KeyEvent` | `KeyEvent.hpp` | Engine key enum, SDL conversion, modifier test, and display names. |
| `Keyboard` | `Keyboard.hpp` | Keyboard state utilities. |
| `Mouse` | `Mouse.hpp` | Mouse state utilities. |

## Shortcuts

| Symbol | Header | Description |
|--------|--------|-------------|
| `Shortcuts::ShortcutData` | `Shortcuts.hpp` | Modifier/key combination plus action handler. |
| `Shortcuts` | `Shortcuts.hpp` | Registers shortcuts and receives key-down events from `MainLoop`. |

Access the active registry through `MainLoop::shortcuts()`.

## Persistence and platform services

| Symbol | Header | Description |
|--------|--------|-------------|
| [`Preferences`](Preferences.md) | `Preferences.hpp` | Typed JSON-backed settings context. |
| [`PreferencesStore`](Preferences.md) | `PreferencesStore.hpp` | Singleton registry for global and scoped preferences. |
| [`FileDialog`](Platform_services.md#filedialog) | `FileDialog.hpp` | Native open, save, and folder dialogs. |
| [`FileHistory`](Platform_services.md#filehistory) | `FileHistory.hpp` | Per-type recent-file lists and filters. |
| [`MessageBox`](Platform_services.md#messagebox) | `MessageBox.hpp` | Native information, warning, error, and custom-button dialogs. |
| `GPUSelectionDialog` | `GPUSelectionDialog.hpp` | Vulkan device selection dialog. |
| `initSdlVideoDriver()` | `SDLUtils.hpp` | Selects the suitable Linux SDL video driver before `SDL_Init`; no-op elsewhere. |

## Offscreen application

| Symbol | Header | Description |
|--------|--------|-------------|
| [`OffscreenApplicationConfig`](OffscreenApplication.md#configuration) | `OffscreenApplication.hpp` | Fixed target dimensions, attachments, and color format. |
| `OffscreenApplicationDelegate` | `OffscreenApplication.hpp` | Offscreen initialization, frame, render, completion, and cleanup callbacks. |
| [`OffscreenApplication`](OffscreenApplication.md) | `OffscreenApplication.hpp` | Engine and fixed-image lifecycle without a window or swapchain. |

## Standalone tools

| Tool | Documentation | Description |
|------|---------------|-------------|
| `lightmap_generator` | [LightmapGenerator](LightmapGenerator.md) | Headless model and prefab lightmap baking using the production render API. |

## Header catalog

| Header | Primary contents |
|--------|------------------|
| `bg2e/app/Application.hpp` | `Application` |
| `bg2e/app/MainLoop.hpp` | `MainLoop`, `WindowConfig`, `SafeUpdateToken` |
| `bg2e/app/InputDelegate.hpp` | `InputDelegate` |
| `bg2e/app/InputManager.hpp` | `InputManager` |
| `bg2e/app/KeyEvent.hpp` | `KeyEvent` |
| `bg2e/app/Keyboard.hpp` | `Keyboard` |
| `bg2e/app/Mouse.hpp` | `Mouse` |
| `bg2e/app/Shortcuts.hpp` | `Shortcuts`, `ShortcutData` |
| `bg2e/app/Preferences.hpp` | `Preferences` |
| `bg2e/app/PreferencesStore.hpp` | `PreferencesStore` |
| `bg2e/app/FileDialog.hpp` | `FileDialog` |
| `bg2e/app/FileHistory.hpp` | `FileHistory` |
| `bg2e/app/MessageBox.hpp` | `MessageBox` |
| `bg2e/app/GPUSelectionDialog.hpp` | `GPUSelectionDialog` |
| `bg2e/app/OffscreenApplication.hpp` | Offscreen application types |
| `bg2e/app/Uv2SafeReload.hpp` | `Uv2SafeReload`, `Uv2RegenerationResult` |
| `bg2e/app/SDLUtils.hpp` | `initSdlVideoDriver()` |
| `bg2e/app/all.hpp` | Umbrella header |
