# Native GUI and window bridges

These bridges isolate native graphics implementation in gpu. They do not depend
on draw, scene, UI sessions, preferences or selection. Their public headers contain
no ImGui or native graphics types; vendor APIs appear only in implementation files.
All three headers are exported by gpu/all.hpp.

| API | Contract |
|---|---|
| `gpu::GuiRenderer` (`gpu/GuiRenderer.hpp`) | Abstract native GUI recording bridge; factories `gpu::vk::createGuiRenderer()` and macOS-only `gpu::metal::createGuiRenderer()`. |
| `initialize(Instance&, PhysicalDevice&, Device&, WindowSurface&)` | Borrows initialized GPU objects and the caller's current, unbound ImGui context. Rejects repeated initialization without shutdown. Does not own an Engine or context. |
| `prepareFrame(CommandBuffer*, SurfaceFrame*)` | Requires an acquired presentation frame and commands without active scopes; prepares backend metadata without opening the overlay pass. |
| `draw(CommandBuffer&, SurfaceFrame&)` | Records a color-only LOAD/STORE overlay after scene work. Does not submit/present. Caller coordinates the same prepared pair. |
| `registerTexture`, `removeTexture`, `textureGeneration` | Uses existing backend preview helpers. Vulkan tracks owned IDs and increments generation on renderer recreation; Metal identifiers retain their existing texture-pointer contract. |
| `shutdown()` | After users stop and GPU work completes, release preview registrations and native renderer state before destroying context/device. Explicit lifecycle, including shutdown after partial initialization; destructor is not implicit synchronization. |
| `gpu::drawableSize(SDL_Window*, BackendType)` (`gpu/WindowGraphics.hpp`) | Native Vulkan/Metal drawable dimensions in pixels. Null window or unavailable backend throws. Zero dimensions remain valid for minimized windows. |
| `gpu::StandaloneGuiWindow` (`gpu/StandaloneGuiWindow.hpp`) | Standalone device-chooser native OpenGL presentation bridge. Constructor takes title and Size2D and borrows the caller's current ImGui context; RAII destroys renderer/platform/context/window and balances SDL subsystem references. Noncopyable. |
| `processEvent`, `prepareFrame`, `present` | For the standalone window, forward SDL events, prepare native backend frames, then present already-rendered ImGui data. Logical widget dimensions stay in app; viewport uses drawable pixels. |

Vulkan derives minimum/image counts and color format from the live swapchain,
drains work before incompatible recreation, retains pool ownership and backend
configuration-pointer lifetime. Metal retains generation/format/sample-count
metadata, autorelease scopes and GPU-owned single encoder termination. Existing
native texture registration helpers remain backend-owned.

UI owns its context, SDL platform integration for the main window, preview registry
and prepared-pair validation. App owns standalone chooser preferences, widgets,
accept/cancel and device properties. The chooser's temporary context is explicitly
selected and restored; its bridge is destroyed first. The standalone bridge also
restores a previous native GL context, if present. Neither bridge changes a running
engine's selected device or introduces a third engine rendering backend.

Compilation and native Vulkan/Metal/OpenGL behavior require project-lead validation;
M09-04 source inspection is not a runtime result.
