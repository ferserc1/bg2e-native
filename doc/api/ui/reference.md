# bg2e::ui API Reference

Complete reference of the public API of the `bg2e::ui` namespace. Classes are
grouped by category; per-class guides with practical recipes are linked from
each section. See [index.md](index.md) for the architecture and
[quick_start.md](quick_start.md) for usage recipes.

All classes are declared inside `namespace bg2e { namespace ui { ... } }` and
exported with `BG2E_API`.

---

## Table of Contents

1. [Core infrastructure](#core-infrastructure)
   - [UserInterface](#userinterface)
   - [UserInterfaceDelegate](#userinterfacedelegate)
   - [DemoWindow](#demowindow)
   - [Loader](#loader)
2. [Windows and layout](#windows-and-layout)
   - [Window](#window)
   - [Workspace](#workspace)
   - [Menu / MenuItem](#menu--menuitem)
   - [Toolbar / ToolbarButton](#toolbar--toolbarbutton)
   - [StatusBar / StatusItem](#statusbar--statusitem)
3. [Primitive widgets](#primitive-widgets)
   - [Layout](#layout)
   - [Text](#text)
   - [Group](#group)
   - [Button](#button)
   - [Numeric](#numeric)
   - [Vector](#vector)
   - [Value](#value)
   - [SelectableList](#selectablelist)
   - [TextureWidgets](#texturewidgets)
4. [Scene editors](#scene-editors)
   - [SceneTree](#scenetree)
   - [NodeEditor](#nodeeditor)
   - [MaterialEditor](#materialeditor)
   - [DrawableEditor](#drawableeditor)
   - [SubmeshSelector](#submeshselector)
   - [LightEditor](#lighteditor)
   - [CameraSettings](#camerasettings)
   - [PolarTransformControllerEditor](#polartransformcontrollereditor)
5. [Reflection widgets](#reflection-widgets)
   - [ReflectionWidget](#reflectionwidget)
   - [ComponentInspector](#componentinspector)
6. [Settings windows](#settings-windows)
   - [UISettingsWindow](#uisettingswindow)
   - [RenderSettingsWindow](#rendersettingswindow)

---

## Core infrastructure

### `UserInterface`

**Header:** `<bg2e/ui/UserInterface.hpp>` · **Guide:** [UserInterface](UserInterface.md)

Owns the ImGui context and its SDL2/Vulkan backends. One instance per
`app::MainLoop` (not copyable, not created by applications).

| Method | Description |
|--------|-------------|
| `void init(render::Engine*)` | Creates command pool/buffer, fence, ImGui context + backends, descriptor pool; loads `uiScale` from the `"ui"` preferences; calls `delegate->init()`. |
| `void processEvent(SDL_Event*)` | Forwards the event to `ImGui_ImplSDL2_ProcessEvent`. Called by `MainLoop` **after** the input manager. |
| `void newFrame()` | Applies pending scale changes, starts the ImGui frame, calls the frame override or `delegate->drawUI()`, then `ImGui::Render()`. |
| `void draw(VkCommandBuffer, VkImageView targetImageView)` | Records the ImGui draw data into `cmd` in its own dynamic-rendering pass over the target (swapchain) image view. |
| `void cleanup()` | Persists `uiScale` to preferences. Vulkan objects are released through the engine `CleanupManager`. |
| `void setFrameOverride(std::function<void()>)` | Replaces the delegate's `drawUI()` with `fn` (used by `MainLoop::asyncLoad`). |
| `void clearFrameOverride()` | Restores the delegate path. |
| `void setDelegate(std::shared_ptr<UserInterfaceDelegate>)` | Installs the application UI delegate. |
| `static float getScale()` | Current global UI scale factor (default `1.0`). |
| `static void setScale(float)` | Sets the scale; style + font are rescaled on the next `newFrame()`. |

Static state: `s_uiScale`, `s_uiFontLoaded`, `s_uiScaleChanged`. The base
style is captured once and re-scaled (never rebuilt), and the engine font
`DidactGothic-Regular.ttf` (16 px) is loaded from the asset path on first use.

### `UserInterfaceDelegate`

**Header:** `<bg2e/ui/UserInterfaceDelegate.hpp>`

Application hook invoked by `UserInterface` each frame. Owned as
`shared_ptr` by both `app::Application` and `UserInterface`.

| Member | Description |
|--------|-------------|
| `virtual void init(render::Engine*, UserInterface*)` | One-shot setup after `UserInterface::init()`. Default: empty. |
| `virtual void drawUI()` | Draw pass hook. **Default implementation draws `DemoWindow`** — override it. |
| `uint32_t uiWidth() const` / `uint32_t uiHeight() const` | Current viewport (SDL window) size. `friend app::MainLoop` keeps them updated. |

### `DemoWindow`

**Header:** `<bg2e/ui/DemoWindow.hpp>`

| Method | Description |
|--------|-------------|
| `static void draw()` | Calls `ImGui::ShowDemoWindow()`. Useful as a widget gallery during development. |

### `Loader`

**Header:** `<bg2e/ui/Loader.hpp>` · **Guide:** [Loader](Loader.md)

Thread-safe modal progress overlay (fixed-size centered window: message +
progress bar). All setters/getters lock a recursive mutex, so worker threads
may update it while the main thread draws it.

| Method | Description |
|--------|-------------|
| `void setMessage(const std::string&)` | Sets the displayed text (default `"Loading..."`). |
| `std::string getMessage() const` | Current message. |
| `void setProgress(float)` | Sets progress, clamped to `[0, 1]`. |
| `float getProgress() const` | Current progress. |
| `void draw()` | Draws the overlay. Main thread only, once per ImGui frame. |

Normally driven via `app::MainLoop::asyncLoad(callback, clearColor)`, which
installs `loader.draw()` as UI frame override while `callback` runs in a
detached thread and resumes the scene when it returns.

---

## Windows and layout

### `Window`

**Header:** `<bg2e/ui/Window.hpp>` · **Guide:** [Window](Window.md)

Wrapper around `ImGui::Begin/End` with declarative options.

| Member | Description |
|--------|-------------|
| `Options options` | See [Window::Options](#windowoptions). |
| `void setTitle(const std::string&)` / `const std::string& title() const` | Window title (must be unique). Untitled windows become `"Window##N"`. |
| `virtual void draw(std::function<void()> drawFunction, std::function<void()> menuFunction = nullptr)` | **Mode 1**: draws the window now with the given body (+ per-call menu bar function). No-op when closed. |
| `virtual void setDrawFunction(std::function<void()>)` | **Mode 2**: stores the body for `draw()`. Ignores null; virtual so `Toolbar`/`StatusBar` can disable it. |
| `void setMenuFunction(std::function<void()>)` | Stores a menu-bar body for `draw()`. Ignores null. |
| `virtual void draw()` | **Mode 2** draw: `draw(_drawFunction)`. |
| `void open()` / `void close()` / `bool isOpen() const` | Visibility toggle; closed windows draw nothing. |
| `void setPosition(int x, int y)` / `int positionX() const` / `int positionY() const` | Position (`-1` = unset → ImGui default). |
| `void setSize(int w, int h)` / `int width() const` / `int height() const` | Size (`-1` = unset). |

#### `Window::Options`

| Field | Default | Maps to |
|-------|---------|---------|
| `noTitleBar` | `false` | `ImGuiWindowFlags_NoTitleBar` |
| `noScrollbar` | `false` | `NoScrollbar` |
| `noMenu` | `false` | unset → `ImGuiWindowFlags_MenuBar` is **enabled** |
| `noMove` | `false` | `NoMove` (+ position re-applied every frame) |
| `noResize` | `false` | `NoResize` (+ size re-applied every frame) |
| `noCollapse` | `false` | `NoCollapse` |
| `noNav` | `false` | `NoNav` |
| `noBackground` | `false` | `NoBackground` |
| `noBringToFront` | `false` | `NoBringToFrontOnFocus` |
| `noClose` | `false` | hides the close button (window stays openable only via `open()`/`close()`) |
| `minWidth/minHeight` | `0` | `SetNextWindowSizeConstraints` lower bound |
| `maxWidth/maxHeight` | `INT_MAX` | upper bound |

#### `Window::DockingSide`

`DockLeft`, `DockRight`, `DockBottom` — enum reserved for docking helpers (not
consumed by `Window` itself).

### `Workspace`

**Header:** `<bg2e/ui/Workspace.hpp>` · **Guide:** [Workspace](Workspace.md)

Fixed docked layout manager for `Window`s (not a `Window` itself).

| Member | Description |
|--------|-------------|
| `void setup(uint32_t width, uint32_t height, Window* toolBar, Window* leftPanel, Window* rightPanel, Window* bottomPanel, Window* statusBar = nullptr)` | Registers the windows (any may be `nullptr`) and positions them. |
| `void resize(uint32_t width, uint32_t height)` | Re-runs the layout for a new viewport. Call from `swapchainResized`. |
| `void draw()` | Draws toolbar + visible panels + status bar; detects UI scale changes. |
| `bool isValid() const` | True once a non-zero viewport is set. |
| `bool toolBarVisible() / leftPanelVisible() / rightPanelVisible() / bottomPanelVisible() / statusBarVisible()` | Per-element visibility queries. |
| `void toggleXxx() / showXxx() / hideXxx() / setXxxVisible(bool)` | Visibility control (`Xxx` ∈ `ToolBar`, `LeftPanel`, `RightPanel`, `BottomPanel`, `StatusBar`); relayouts immediately. |
| `PanelSize& leftPanelSize() / rightPanelSize() / bottomPanelSize()` | Panel size constraints (by reference — mutate in place). |

#### `Workspace::PanelSize`

| Field | Meaning |
|-------|---------|
| `uint32_t min` | Minimum logical size (px). |
| `uint32_t max` | Maximum logical size (px). |
| `float relative` | Fraction of the viewport (`clamp(vp * relative, min, max)`). |

Defaults: left/right `{150, 500, 0.15}`, bottom `{100, 400, 0.20}`; toolbar
height 60 px, status bar height 50 px (both scaled). `setup()` snapshots the
UI scale; `draw()` relayouts when `UserInterface::getScale()` changes.

### `Menu` / `MenuItem`

**Header:** `<bg2e/ui/Menu.hpp>` · **Guide:** [Menu](Menu.md)

#### `MenuItem`

Data-driven menu entry (action, checkable action, separator or submenu).

| Member | Description |
|--------|-------------|
| `MenuItem()` / `MenuItem(const std::string& label)` | Separator (label kept for submenus). |
| `MenuItem(label, ShortcutData)` | Action; `ShortcutData` (from `app::Shortcuts`) carries modifiers, key and `handler`. |
| `MenuItem(label, ShortcutData, std::function<bool()> isChecked)` | Checkable action polled every frame. |
| `std::string label` | Displayed text. |
| `void addMenuItem(const MenuItem&)` | Turns the item into a **submenu** and appends the child. |
| `void draw() const` | Renders the item (recursive for submenus). |
| `void initShortcuts() const` | Registers keyboard shortcuts with `MainLoop::current()->shortcuts()` (recursive; called once by `Menu::draw`). |

`MenuItemType` (`Action`, `CheckableAction`, `Separator`, `Submenu`) is a
plain (unscoped) enum; the type is inferred from the used constructor /
`addMenuItem`.

#### `Menu`

| Static | Description |
|--------|-------------|
| `bool beginMenuBar()` / `void endMenuBar()` | Window menu-bar scope. |
| `bool beginMenu(label, enabled = true)` / `void endMenu()` | Submenu scope (true when open). |
| `bool menuItem(label, shortcut = "", selected = false, enabled = true)` | One item; returns true on activation. `shortcut` is *display-only* text here. |
| `void separator()` | Horizontal separator. |

| Instance | Description |
|----------|-------------|
| `void addMenuItem(const MenuItem&)` | Appends to the tree. |
| `void draw()` | Draws all items (call inside a menu-bar scope); installs shortcuts on first call. |

### `Toolbar` / `ToolbarButton`

**Header:** `<bg2e/ui/Toolbar.hpp>` · **Guide:** [Toolbar & StatusBar](Toolbar_and_StatusBar.md)

#### `ToolbarButton`

| Field | Description |
|-------|-------------|
| `int32_t id` | Toolbars-wide unique id; `< 0` on `addButton` ⇒ auto-generated. |
| `std::string label` | Button caption, or plain text when `action == nullptr`. |
| `std::function<void()> action` | Click handler. |
| `bool disabled` | Greyed + non-interactive. |

#### `Toolbar : public Window`

| Member | Description |
|--------|-------------|
| `int32_t addButton(ToolbarButton&, Alignment = AlignLeft)` | Adds (copying; may rewrite `button.id`) and returns the final id. |
| `int32_t addButton(ToolbarButton&&, Alignment = AlignLeft)` | Move overload. |
| `void draw() override` | Draws left-aligned buttons, then right-aligned buttons/text. |
| `void updateButtonLabel(int32_t id, const std::string& / std::string&&)` | Live caption change. |
| `void enableButton(int32_t id)` / `void disableButton(int32_t id)` | Live enable state. |
| `ToolbarButton* findButton(int32_t id)` | Lookup (`nullptr` when absent). |
| `void addMenuItem(const MenuItem&)` | Forwards to the internal `Menu` (drawn in the window menu bar). |
| `enum Alignment` | `AlignLeft` / `AlignRight`. |

`setDrawFunction()` and the two-argument `draw()` are overridden and do not
accept custom bodies (the toolbar body is fixed); the menu function is wired
automatically on the first draw.

### `StatusBar` / `StatusItem`

**Header:** `<bg2e/ui/StatusBar.hpp>` · **Guide:** [Toolbar & StatusBar](Toolbar_and_StatusBar.md)

#### `StatusItem`

| Member | Description |
|--------|-------------|
| `void setText(const std::string&)` / `void setText(std::string&&)` | Update the displayed text (call anytime; next frame reflects it). |
| `const std::string& getText() const` | Current text. |

#### `StatusBar : public Window`

| Member | Description |
|--------|-------------|
| `void addItem(std::shared_ptr<StatusItem>, Alignment = AlignLeft)` | Appends an item to the left or right group. |
| `void draw() override` | Draws vertically-centered left text, then right-aligned text. |
| `enum Alignment` | `AlignLeft` / `AlignRight`. |

`setDrawFunction()` is a no-op, same as `Toolbar`.

---

## Primitive widgets

### `Layout`

**Header:** `<bg2e/ui/Layout.hpp>` · **Guide:** [Layout](Layout.md)

Static-only placement, child regions and size metrics.

| Method | Description |
|--------|-------------|
| `static void sameLine(int32_t xPos = 0)` | In-line placement. `xPos > 0`: absolute offset from left; `xPos < 0`: offset **from the right window edge**; `0`: natural next-item position. |
| `static void spacing(int32_t spacing = 20)` | Vertical gap (dummy item). |
| `static void padding(uint32_t width, uint32_t height)` | Empty rectangle placeholder. |
| `static float getContentRegionAvailWidth() / getContentRegionAvailHeight()` | Remaining space in the current window/child. |
| `static void beginChild(const std::string& id, float width = 0, float height = 0, bool border = true)` / `static void endChild()` | Child region (0 = fill; independent scroll). |
| `static uint32_t calcTextWidth / calcTextHeight(const std::string&)` | Text measurement. |
| `static uint32_t calcButtonWidth / calcButtonHeight(const std::string&)` | Button frame measurement (text + frame padding). |
| `static uint32_t getItemHorizontalSpacing() / getItemVerticalSpacing()` | Style `ItemSpacing`. |

### `Text`

**Header:** `<bg2e/ui/Text.hpp>` · **Guide:** [Text](Text.md)

Static-only non-interactive display widgets.

| Method | Description |
|--------|-------------|
| `static void text(const std::string&, bool sameLine = false)` | Non-editable label. |
| `static void separator(const std::string& title = "", bool sameLine = false)` | Section separator with optional inline title. |
| `static void listItem(const std::string&, bool sameLine = false)` | Bulleted line. |
| `static void tooltip(const std::string& text)` | Hover tooltip for the last item (no-op on empty text). |

### `Group`

**Header:** `<bg2e/ui/Group.hpp>` · **Guide:** [Group](Group.md)

Static-only scoped helpers (begin/end or push/pop pairs).

| Method | Description |
|--------|-------------|
| `static bool beginTree(const std::string& label)` | Collapsible tree node (default open); pair with `endTree()` when it returns true. |
| `static void endTree()` | Closes a tree. |
| `static bool collapsingHeader(const std::string& title, bool visible = true)` | One-line header; `visible == true` starts it open. No pairing needed. |
| `static void beginDisabled(bool disabled = true)` / `static void endDisabled()` | Disable a group of controls. |
| `static void pushId(int id)` / `static void popId()` | ImGui ID stack (repeated labels). |

### `Button`

**Header:** `<bg2e/ui/Button.hpp>` · **Guide:** [Button](Button.md)

Static-only button family.

| Method | Description |
|--------|-------------|
| `static bool button(const std::string& title, bool sameLine = false, bool disabled = false)` | Returns true on click. |
| `static bool checkBox(const std::string& title, bool* value = nullptr, bool sameLine = false, bool disabled = false)` | Returns true on change; writes through `value`. |
| `static bool radioButton(const std::string& label, int* value = nullptr, int id = 0, bool sameLine = false, bool disabled = false)` | Writes `id` into `*value` when clicked. |

### `Numeric`

**Header:** `<bg2e/ui/Numeric.hpp>` · **Guide:** [Numeric](Numeric.md)

Static-only scalar value editors. All return `true` on the frame the value
changes and modify the passed value in place. All accept a trailing
`sameLine`.

| Method | Notes |
|--------|-------|
| `bool number(label, int*/float*/double* value, ...)` | Stepped numeric input. |
| `bool slider(label, int* value, min = 0, max = 100, ...)`, `bool slider(label, float* value, min = 0, max = 1, ...)` | Range slider. |
| `bool sliderInt / sliderFloat / sliderDouble(label, value, min, max, ...)` | Typed aliases; `sliderDouble` slides as float. |
| `bool drag(label, float* value, speed = 0.1f, min = 0, max = 0, ...)`, `bool drag(label, int* value, speed = 1.0f, min = 0, max = 0, ...)` | Drag-to-adjust; `min == max == 0` ⇒ unclamped. |

### `Vector`

**Header:** `<bg2e/ui/Vector.hpp>` · **Guide:** [Vector](Vector.md)

Static-only vector and matrix editors. All return `true` on the frame the
value changes and modify the passed value in place.

| Method | Notes |
|--------|-------|
| `bool vec2/vec3/vec4(label, int*/float* value, ...)` | Raw-array components. |
| `bool vec2/vec3/vec4(label, glm::vec2&/vec3&/vec4& value, ...)` | GLM overloads (copy in / copy out on change). |
| `bool mat4(const std::string& label, glm::mat4& value, ...)` | Position / Rotation(deg) / Scale editor; internal **per-label** euler cache (see [Vector](Vector.md)). |

### `Value`

**Header:** `<bg2e/ui/Value.hpp>` · **Guide:** [Value](Value.md)

Static-only non-numeric value editors. All return `true` on the frame the
value changes and modify the passed value in place. All accept a trailing
`sameLine`.

| Method | Notes |
|--------|-------|
| `bool text(const std::string& label, std::string& value, int maxLength = 200, ...)` | Editable text field. |
| `bool textWithHint(label, hint, std::string& value, maxLength = 200, ...)` | Placeholder when empty. |
| `bool colorPicker(label, base::Color& color, ...)` | RGBA color edit (`ColorEdit4`). |
| `bool comboBox(label, const std::vector<std::string>& items, uint32_t& selected, ..., bool fitPreview = false)` | Dropdown; entries displayed as `"idx: label"`; `selected` clamped into range. |
| `bool comboBox(label, ItemListCallback, uint32_t& selected, ...)` | Callback form: `void(std::vector<std::string>&)` refills the items each frame (dynamic lists). |

### `SelectableList`

**Header:** `<bg2e/ui/SelectableList.hpp>`

Static-only table of selectable rows.

| Method | Description |
|--------|-------------|
| `static void beginList(int columns = 1)` | Opens the bordered ImGui table (settings not persisted). |
| `static bool item(const std::string& title, bool& selected)` | New row + first column; returns true on click and toggles `selected` through ImGui. |
| `static bool itemButton(const std::string& title)` | Next column: a button; returns true on click. |
| `static void endList()` | Closes the table. |

### `TextureWidgets`

**Header:** `<bg2e/ui/TextureWidgets.hpp>` · **Guide:** [TextureWidgets](TextureWidgets.md)

Displays one `render::Texture` slot as ImGui image. Holds a
`VkDescriptorSet` created via `ImGui_ImplVulkan_AddTexture`.

| Method | Description |
|--------|-------------|
| `void setEditTexture(std::shared_ptr<render::Texture>)` | Rebinds the displayed texture **now** (waits for GPU idle when releasing the old descriptor set). Do not call mid-frame. |
| `void setDeferredTexture(std::shared_ptr<render::Texture>)` | Queues the swap for the next `drawImage`/`imageButton` call (frame-safe). |
| `void clearTexture()` | Unbinds the texture (descriptor released, waitIdle). |
| `void drawImage(uint32_t width, uint32_t height, bool sameLine = false)` | Plain image preview. |
| `bool imageButton(const std::string& id, uint32_t w, uint32_t h, bool sameLine = false)` | Clickable image; returns true on click. No-op (false) while no texture is bound. |
| `bool selectTexture(const std::string& label, std::function<std::shared_ptr<render::Texture>(base::Texture*)> cb)` | Image-button + file dialog + Clear button row. The callback receives the loaded `base::Texture*` (**takes ownership**, `nullptr` on Clear) and returns the `render::Texture` to display. Returns true when a file was picked. |
| `void cleanup()` | Releases descriptor + texture references. |

---

## Scene editors

All scene editors share the same shape: `setXxxComponent(...)` /
`getXxxComponent()` (weak pointer), `bool draw()` (true on change),
`void cleanup()`, and `void onChanged(std::function<void()>)`.

### `SceneTree`

**Header:** `<bg2e/ui/SceneTree.hpp>` · **Guide:** [Scene editors](SceneEditors.md)

| Method | Description |
|--------|-------------|
| `void setRootNode(scene::Node* root)` / `scene::Node* rootNode() const` | Hierarchy to display. |
| `void setSelectionManager(manipulation::SelectionManager*)` / getter | Selection source of truth. `nullptr` ⇒ read-only tree. |
| `void draw()` | Draws the tree; clicks drive the selection manager (Ctrl = additive when multi-selection is enabled). |

### `NodeEditor`

**Header:** `<bg2e/ui/NodeEditor.hpp>` · **Guide:** [Scene editors](SceneEditors.md)

| Method | Description |
|--------|-------------|
| `void init(render::Engine*)` | Engine used by the drawable "Replace Model" action. |
| `void setNode(scene::Node*)` / `void setNodes(const std::vector<scene::Node*>&)` | Single or multi selection; > 1 node draws a placeholder. |
| `scene::Node* node() const` | Current node. |
| `void draw()` | Name, enabled flag, component list, and per-component editors. |
| `void onChanged(ChangedCallback)` | Invoked when any section edits the node. |

Owns (and reuses) a `LightEditor`, `PolarTransformControllerEditor` and
`CameraSettings`; the transform section keeps a cached euler set keyed by the
current node.

### `MaterialEditor`

**Header:** `<bg2e/ui/MaterialEditor.hpp>` · **Guide:** [Material & Drawable editors](Material_Editors.md)

| Method | Description |
|--------|-------------|
| `void setEditMaterial(std::shared_ptr<render::MaterialBase>&)` | Manual mode: replace the edit list with one material. Ignored with a selection manager. |
| `void addEditMaterial(std::shared_ptr<render::MaterialBase>&)` | Manual mode: append to the multi-edit list. |
| `void clearMaterial()` | Manual mode: drop the edit target. |
| `void setSelectionManager(const std::shared_ptr<manipulation::SelectionManager>&)` | Selection-driven mode; installs an `onSelect` hook (first selected drawable + all submesh materials of that drawable). |
| `std::shared_ptr<render::MaterialBase> editMaterial()` | Main (first) material being edited. |
| `bool draw()` | Full PBR form; edits are applied to **all** materials of the list. |
| `void cleanup()` | Releases the six texture widgets. |
| `void onChanged(std::function<void()>)` | Fired per edited material. |

Sections: Albedo (color, texture, scale, UV set, transparency, refraction),
Normal (texture, scale, UV), Metallic / Roughness (value, texture, channel,
invert, scale, UV), Fresnel Tint, Sheen (intensity + color), Ambient
Occlusion, Light Emission.

### `DrawableEditor`

**Header:** `<bg2e/ui/DrawableEditor.hpp>` · **Guide:** [Material & Drawable editors](Material_Editors.md)

| Method | Description |
|--------|-------------|
| `void init(const std::shared_ptr<manipulation::SelectionManager>&)` | Binds the internal `SubmeshSelector`. |
| `int32_t selectedItem() const` / `std::vector<uint32_t> selectedItems() const` | Submesh selection (delegates to the selector). |
| `SubmeshSelector& submeshSelector()` | Direct access. |
| `bool draw()` | Collapsing header + submesh list + name/group/visibility fields. |
| `void cleanup()` / `void onChanged(cb)` | Standard. |

### `SubmeshSelector`

**Header:** `<bg2e/ui/SubmeshSelector.hpp>` · **Guide:** [Material & Drawable editors](Material_Editors.md)

Edits the submeshes of the **first** selected item owning a drawable.

| Method | Description |
|--------|-------------|
| `void init(std::shared_ptr<manipulation::SelectionManager>)` | Installs the `onSelect` hook that snapshots the current drawable selection. |
| `std::shared_ptr<scene::Drawable> editDrawable()` | The drawable being edited (`nullptr` if none). |
| `int32_t selectedItem() const` | First selected submesh index or `-1`. |
| `std::vector<uint32_t> selectedItems() const` | All selected submesh indices of the edit drawable. |
| `void addSelectedItem(uint32_t index) const` | Programmatic addition to the selection. |
| `bool draw()` | Submesh list (toggles selection) + "Clear Selection" button. |
| `void cleanup()` | Standard. |

### `LightEditor`

**Header:** `<bg2e/ui/LightEditor.hpp>` · **Guide:** [Scene editors](SceneEditors.md)

| Method | Description |
|--------|-------------|
| `void setLightComponent(scene::LightComponent*)` / `(const std::shared_ptr<...>&)` | Component to edit (weakly stored). |
| `std::weak_ptr<scene::LightComponent> getLightComponent() const` | Current component. |
| `void setIntensityRange(float min, float max)` | Slider range (default 0–30). |
| `bool draw()` | Type combo, color, intensity, shadows (+ source size / samples), spot cone when `TypeSpot`. |
| `void cleanup()` / `void onChanged(cb)` | Standard. |

### `CameraSettings`

**Header:** `<bg2e/ui/CameraSettings.hpp>` · **Guide:** [Scene editors](SceneEditors.md)

| Method | Description |
|--------|-------------|
| `void setCameraComponent(scene::CameraComponent*)` / `(const std::shared_ptr<...>&)` | Component to edit; syncs focal length / frame size from its `OpticalProjection` (or defaults 35 mm). |
| `std::weak_ptr<scene::CameraComponent> getCameraComponent() const` | Current component. |
| `bool draw()` | If the camera has no `OpticalProjection`: "Configure Projection" button. Otherwise focal-length slider (18–200 mm) + sensor-size combo. |
| `static const std::vector<std::string>& sensorNames()` | `APS-C (22mm)`, `Full Frame (35mm)`, `Medium Format (50mm)`, `Large Format (75mm)`. |
| `static float sensorSize(uint32_t index)` | Frame size in mm per preset. |
| `static uint32_t getSensorIndex(float frameSize)` | Nearest preset for an arbitrary frame size. |
| `void cleanup()` / `void onChanged(cb)` | Standard. |

### `PolarTransformControllerEditor`

**Header:** `<bg2e/ui/PolarTransformControllerEditor.hpp>` · **Guide:** [Scene editors](SceneEditors.md)

| Method | Description |
|--------|-------------|
| `void setComponent(scene::PolarTransformControllerComponent*)` / shared_ptr overload | Component to edit. |
| `bool draw()` | Position (azimuth 0–360, elevation −90–90, distance 0–50), Orientation (euler X/Y/Z −360–360), Target vec3. |
| `void cleanup()` / `void onChanged(cb)` | Standard. |

---

## Reflection widgets

### `ReflectionWidget`

**Header:** `<bg2e/ui/ReflectionWidget.hpp>` · **Guide:** [Reflection widgets](Reflection_Widgets.md)

Static-only generic form generated from `bg2e::reflection` metadata.

| Method | Description |
|--------|-------------|
| `static bool drawProperties(void* instance, const reflection::TypeInfo& info, uint32_t depth = 0)` | Draws all properties grouped by category; returns true when any value changed. Object properties recurse as trees up to `reflection::maxObjectDepth`. |
| `static void drawActions(void* instance, const reflection::TypeInfo& info)` | One button per reflected action. |

Widget selection per property type/editor: `Bool`→checkbox, numeric→input /
slider / drag (honoring `min`/`max`/`step`), `Angle` editor→slider (with
range) or drag, `String`→text, `VecN`/`Mat4`→matching `Vector`
editors, `Color`→`Value::colorPicker`, read-only→disabled group + tooltip.
`Enum`, `Path` and `Resource` fall back to labels (see the guide for details).

### `ComponentInspector`

**Header:** `<bg2e/ui/ComponentInspector.hpp>` · **Guide:** [Reflection widgets](Reflection_Widgets.md)

| Method | Description |
|--------|-------------|
| `void setNode(scene::Node*)` / `scene::Node* node() const` | Node to inspect (`nullptr` ⇒ "No selection"). |
| `void draw()` | Collapsing header per component (display name from `TypeInfo`, fallback `typeName()`), reflected properties + actions, and a right-aligned **Remove** button per component (deferred until after iteration). |
| `void onChanged(std::function<void()>)` | Fired on property change or component removal. |

---

## Settings windows

### `UISettingsWindow`

**Header:** `<bg2e/ui/UISettingsWindow.hpp>` · **Guide:** [Settings windows](Settings_Windows.md)

`Window` subclass, initially closed. `init()` sets title/size and the draw
body: interface-scale slider (1.0–2.0) + per-`GizmoType` visibility /
opacity / scale controls (camera, point/spot/directional light, environment,
transform + uniform/axis scale handles).

### `RenderSettingsWindow`

**Header:** `<bg2e/ui/RenderSettingsWindow.hpp>` · **Guide:** [Settings windows](Settings_Windows.md)

`Window` subclass, initially closed.

| Method | Description |
|--------|-------------|
| `void init(render::RendererDeferred*, render::RenderSettingsPreferences*)` | Binds the deferred renderer and its preferences store; every change is written back to `_prefs` (persisted by the application). |

Sections: render scale (combo of `RendererDeferred::scaleOptions`), indirect
lighting mode (RTAO / RTGI), RT reflections, temporal accumulator, denoise
filter.
