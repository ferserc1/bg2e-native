# bg2e::ui Quick Start Guide

A practical, recipe-oriented guide to `bg2e::ui`. Each recipe is a small
self-contained task focused on the widgets that are **not** trivial to use:
the window/workspace system, data-driven menus and toolbars, texture widgets,
the selection-driven editors, the reflection widgets and the loader. Simple
labels and buttons barely need a guide — everything else is here.

Working code ships in:

- `examples/02_ui/src/main.cpp` — minimal delegate + `Window` demo.
- `apps/model_edit/src/AppDelegate.cpp` — complete workspace application.

---

## Table of Contents

1. [Include the module](#include-the-module)
2. [Recipe 1: Wire a UI into an application](#recipe-1-wire-a-ui-into-an-application)
3. [Recipe 2: A simple window with a draw lambda](#recipe-2-a-simple-window-with-a-draw-lambda)
4. [Recipe 3: Build an editor layout with Workspace](#recipe-3-build-an-editor-layout-with-workspace)
5. [Recipe 4: Menus with keyboard shortcuts](#recipe-4-menus-with-keyboard-shortcuts)
6. [Recipe 5: Toolbar buttons with live state](#recipe-5-toolbar-buttons-with-live-state)
7. [Recipe 6: Status bar with live-updating items](#recipe-6-status-bar-with-live-updating-items)
8. [Recipe 7: Selectable lists](#recipe-7-selectable-lists)
9. [Recipe 8: Input widgets and the mat4 trap](#recipe-8-input-widgets-and-the-mat4-trap)
10. [Recipe 9: Texture widgets (and deferred texture swaps)](#recipe-9-texture-widgets-and-deferred-texture-swaps)
11. [Recipe 10: Material editor driven by a SelectionManager](#recipe-10-material-editor-driven-by-a-selectionmanager)
12. [Recipe 11: Drawable / submesh editing](#recipe-11-drawable--submesh-editing)
13. [Recipe 12: Scene tree selection](#recipe-12-scene-tree-selection)
14. [Recipe 13: Reflection-driven inspectors](#recipe-13-reflection-driven-inspectors)
15. [Recipe 14: Modal loader during async work](#recipe-14-modal-loader-during-async-work)
16. [Common pitfalls](#common-pitfalls)

---

## Include the module

```cpp
#include <bg2e/ui/all.hpp>          // everything (recommended)
```

Individual headers:

```cpp
#include <bg2e/ui/UserInterface.hpp>   // UserInterface + delegate
#include <bg2e/ui/BasicWidgets.hpp>    // labels, buttons, layout
#include <bg2e/ui/Window.hpp>          // floating windows
#include <bg2e/ui/Workspace.hpp>       // editor layout
#include <bg2e/ui/Menu.hpp>            // menu bar + shortcut items
#include <bg2e/ui/Toolbar.hpp>         // toolbar window
#include <bg2e/ui/StatusBar.hpp>       // status bar window
#include <bg2e/ui/Input.hpp>           // value editors
#include <bg2e/ui/SelectableList.hpp>  // selectable tables
#include <bg2e/ui/TextureWidgets.hpp>  // texture previews / pickers
#include <bg2e/ui/MaterialEditor.hpp>  // material inspector
#include <bg2e/ui/DrawableEditor.hpp>  // drawable/submesh inspector
#include <bg2e/ui/SceneTree.hpp>       // node hierarchy widget
#include <bg2e/ui/NodeEditor.hpp>      // node inspector
#include <bg2e/ui/ReflectionWidget.hpp>   // generic reflected-type form
#include <bg2e/ui/ComponentInspector.hpp> // generic component inspector
#include <bg2e/ui/Loader.hpp>          // modal progress window
```

**Key points:**
- The module is Layer 8: it depends on `render`, `scene`, `manipulation`,
  `reflection`, `app`, `base`, `math`. Include it only from application/UI code.
- ImGui itself is **not** re-exported: engine widgets are enough for typical
  layouts, and headers like `imgui.h` are private to the `.cpp` files.

---

## Recipe 1: Wire a UI into an application

You never create a `UserInterface` yourself — `app::MainLoop` owns one. You
only implement `UserInterfaceDelegate` (usually together with the render and
input delegates) and register it with the `Application`:

```cpp
class MyDelegate : public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>,
                   public bg2e::app::InputDelegate,
                   public bg2e::ui::UserInterfaceDelegate
{
public:
    void init(bg2e::render::Engine* engine, bg2e::ui::UserInterface* ui) override
    {
        // Called once after UserInterface::init(). The viewport size is
        // already available through uiWidth() / uiHeight().
        _myWindow.setTitle("My Panel");
        _myWindow.setPosition(10, 60);
    }

    void drawUI() override
    {
        _myWindow.draw();          // must be called every frame
    }

protected:
    bg2e::ui::Window _myWindow;
};

class MyApp : public bg2e::app::Application {
public:
    void init(int argc, char** argv) override
    {
        auto delegate = std::shared_ptr<MyDelegate>(new MyDelegate());
        setRenderDelegate(delegate);
        setInputDelegate(delegate);
        setUiDelegate(delegate);   // <- the UI part
    }
};

int main(int argc, char** argv)
{
    bg2e::app::MainLoop mainLoop("org.example.myapp");
    MyApp app;
    app.init(argc, argv);
    return mainLoop.run(&app);
}
```

**Key points:**
- `UserInterfaceDelegate`'s default `drawUI()` draws the ImGui demo window —
  **override it**, or you will get an unexpected "Dear ImGui Demo" window.
- The delegate is held as `shared_ptr` by both `Application` and
  `UserInterface`, so it must outlive the frame loop; capture `this` freely
  inside lambdas that live in the delegate.
- `UserInterface::processEvent()` runs **after** the input manager, so scene
  input handlers must ignore events that the UI has already consumed.

---

## Recipe 2: A simple window with a draw lambda

`Window` supports two usage modes. The quick mode passes the body as a lambda
on each call; the persistent mode configures it once and calls `draw()` with
no arguments:

```cpp
// Mode 1: lambda at draw time (state is static/thread_local or lives in the delegate)
void drawUI() override
{
    _window.draw([this]() {
        ui::BasicWidgets::text("Hello, world!");
        if (ui::BasicWidgets::button("Click Me")) {
            ++_clicks;
        }
    });
}

// Mode 2: configure once, then draw() with no arguments
void init(bg2e::render::Engine*, ui::UserInterface*) override
{
    _panel.setTitle("Document");
    _panel.options.minWidth = 250;
    _panel.options.noCollapse = true;
    _panel.setDrawFunction([this]() {
        ui::BasicWidgets::text(_document->name());
    });
}

void drawUI() override
{
    _panel.draw();          // no-op while _panel.isOpen() is false
}

void keyUp(const app::KeyEvent& e) override
{
    if (e.key() == app::KeyEvent::KeyD) {
        _panel.isOpen() ? _panel.close() : _panel.open();
    }
}
```

**Key points:**
- `close()` does not destroy anything: it only stops drawing the window.
  Reopen it with `open()`. This is the idiom for togglable panels (bind it to
  a menu item).
- Windows without an explicit title get `"Window##N"` (N is a global counter)
  because ImGui requires unique window names.
- Position and size are applied with `ImGuiCond_FirstUseEver` *unless* the
  matching `noMove`/`noResize` option is set — in that case the engine
  re-applies them **every frame**, which is exactly what `Workspace` relies on
  (see next recipe).
- The menu bar of a window is only drawn when a menu function is set (either
  as `draw()` argument or via `setMenuFunction()`); keep `options.noMenu`
  false (the default) if you plan to use it.

---

## Recipe 3: Build an editor layout with Workspace

`Workspace` lays out up to five `Window`s in a fixed docked arrangement:
toolbar (top), left panel, right panel, bottom panel, status bar (bottom). It
is *not* a free docking system — the windows are repositioned and their
`Window::Options` are **overwritten** by the workspace on every
setup/resize/scale change.

```cpp
void initWorkspace()
{
    // 1. Optional: tune panel sizes (logical pixels, scaled at runtime)
    _workspace.leftPanelSize()  = { .min = 300, .max = 600, .relative = 0.20f };
    _workspace.rightPanelSize() = { .min = 200, .max = 500, .relative = 0.15f };
    // Pass a nullptr panel/skip it to leave it out of the layout.

    // 2. Every window managed by the workspace MUST have a draw function
    //    (setDrawFunction / Toolbar & StatusBar already have one).
    _leftPanel.setDrawFunction([this]() { _drawableEditor.draw(); });
    _rightPanel.setDrawFunction([this]() { _environment.draw(); });

    // 3. Setup with the current viewport size
    _workspace.setup(
        uiWidth(), uiHeight(),
        &_toolBar,        // ui::Toolbar*
        &_leftPanel,      // ui::Window*  (scene tree)
        &_rightPanel,     // ui::Window*  (inspector)
        nullptr,          // ui::Window*  (bottom panel, optional)
        &_statusBar       // ui::StatusBar* (optional)
    );
}

// 4. Keep the layout in sync with the swapchain
void swapchainResized(VkExtent2D extent) override
{
    DefaultRenderLoopDelegate::swapchainResized(extent);
    _workspace.resize(uiWidth(), uiHeight());
}

void drawUI() override
{
    _workspace.draw();    // draws toolbar, panels and status bar
}
```

**Key points:**
- Do **not** set `Window::options`, position or size on workspace-managed
  windows: `updateWindows()` overwrites them (`noMove`, `noResize`,
  `noClose`, ...). The toolbar additionally loses its title bar, the panels
  keep theirs (so they can be recognized when floating).
- Panels are sized `clamp(viewport * relative, min, max)` in logical pixels,
  then multiplied by `UserInterface::getScale()`. Adjust through the
  `leftPanelSize()` / `rightPanelSize()` / `bottomPanelSize()` accessors.
- Visibility is controlled per element: `toggleLeftPanel()`, `hideToolBar()`,
  `setStatusBarVisible(false)`, ... — bind these to menu items bound to
  shortcuts. Toggling triggers `updateWindows()`, so geometry stays coherent.
- `Workspace::draw()` detects scale changes by itself and recomputes the
  layout; you only need to call `resize()` when the viewport changes.
- Workspace windows must have **unique titles** (the default `Window##N`
  counter already guarantees it). Two panels sharing a title break both.

---

## Recipe 4: Menus with keyboard shortcuts

Two levels of abstraction live in `Menu.hpp`:

**Low level** — call the static primitives inside a window menu-bar function:

```cpp
_window.draw(body, []() {
    if (ui::Menu::beginMenu("File"))
    {
        if (ui::Menu::menuItem("Open...", "Ctrl+O")) {
            openDocument();
        }
        ui::Menu::separator();
        if (ui::Menu::menuItem("Quit", "Ctrl+Q")) {
            app::MainLoop::current()->exit();
        }
        ui::Menu::endMenu();
    }
});
```

**Data driven** — build a `MenuItem` tree once; it handles rendering **and**
registers the keyboard shortcuts with `app::Shortcuts`:

```cpp
using Shortcut = app::Shortcuts::ShortcutData;

void init(ui::UserInterface*) override
{
    MenuItem fileMenu("File");
    fileMenu.addMenuItem(MenuItem("Toggle Left Panel",
        Shortcut { .ctrlModifier = true, .key = app::KeyEvent::KeyL,
                   .handler = [this]() { _workspace.toggleLeftPanel(); } }));
    fileMenu.addMenuItem(MenuItem());   // default-constructed => separator

    _menu.addMenuItem(fileMenu);
}

// In the toolbar/window menu function (Window opens the menu bar itself):
setMenuFunction([this]() {
    _menu.draw();               // draws + installs shortcuts (first draw only)
});
```

Checkable entries poll a predicate each frame:

```cpp
MenuItem viewMenu("View");
viewMenu.addMenuItem(MenuItem(
    "Left Panel",
    Shortcut { .ctrlModifier = true, .key = app::KeyEvent::KeyE,
               .handler = [this]() { _workspace.toggleLeftPanel(); } },
    [this]() { return _workspace.leftPanelVisible(); }));   // checked state
```

**Key points:**
- The `ShortcutData` handler is invoked **both** from the menu click and from
  the keyboard — write it as the single source of truth for the command.
- Shortcuts are registered lazily: `Menu::draw()` calls `initShortcuts()` on
  the first frame only, via `app::MainLoop::current()->shortcuts()`. Therefore
  a `Menu`/`MenuItem` can only be used inside a running `MainLoop`, and items
  added *after* the first draw will show up but **won't** register their
  shortcut.
- On macOS prefer `ctrlModifier` semantics carefully: the engine maps the key
  modifiers as-is; `getShortcutString()` renders the label ("Ctrl+O", etc.).
  Use a key different from plain characters consumed by text inputs.
- `MenuItem(label)` is a submenu *container*: it becomes a real submenu as
  soon as you call `addMenuItem()` on it (and draws as a separator if you
  never add children). A default-constructed `MenuItem()` is a separator. To
  make an action you must pass a `ShortcutData`.

---

## Recipe 5: Toolbar buttons with live state

`Toolbar` is a `Window` subclass with a left/right button layout. Buttons are
identified by `int32_t` ids so you can update them later:

```cpp
ui::ToolbarButton undoBtn {
    .id = -1,                     // < 0 => the Toolbar generates a unique id
    .label = "Undo",
    .action = [this]() { _document->undo(); },
};
int32_t undoId = _toolBar.addButton(std::move(undoBtn));
int32_t fpsId  = _toolBar.addButton({ .label = "60 fps", .action = nullptr },
                                   ui::Toolbar::AlignRight);
```

The id survives for the lifetime of the toolbar; use it to react to state
changes outside the UI thread of logic:

```cpp
void onUndoStackChanged()
{
    _toolBar.updateButtonLabel(undoId, "Undo (" + std::to_string(_undoCount) + ")");
    _undoCount > 0 ? _toolBar.enableButton(undoId) : _toolBar.disableButton(undoId);
}
```

**Key points:**
- A button with `action == nullptr` renders as **plain text** — handy for the
  right-aligned readouts (fps, statistics) that share the bar with buttons.
- `addButton()` may modify `button.id` when it auto-generates one (the
  by-reference overload), or consumes it (the rvalue overload). Always take
  the **returned** id as authoritative.
- Right-side alignment is computed from text/button widths every frame, so
  dynamic labels re-flow correctly. Very wide labels can overlap left-side
  buttons — keep right-side labels short.
- `Toolbar` ignores `setDrawFunction()` (the body is fixed) but forwards its
  menu to an internal `Menu`: `_toolBar.addMenuItem(...)` (see
  [Recipe 4](#recipe-4-menus-with-keyboard-shortcuts)).

---

## Recipe 6: Status bar with live-updating items

`StatusBar` holds `shared_ptr<StatusItem>`s — keep your own handle and call
`setText()` whenever something changes; the bar repaints next frame:

```cpp
// Setup (once):
_fileStatus = std::make_shared<ui::StatusItem>();
_saveStatus = std::make_shared<ui::StatusItem>();
_statusBar.addItem(_fileStatus, ui::StatusBar::AlignLeft);
_statusBar.addItem(_saveStatus, ui::StatusBar::AlignRight);

// Later, from anywhere (document code, undo stack, async callbacks):
void Document::updateStatus()
{
    _fileStatus->setText(_path.filename().string());
    _saveStatus->setText(_unsaved ? "Unsaved changes" : "Saved");
}
```

**Key points:**
- Items are right-aligned in the order they were added, measured with
  `BasicWidgets::calcTextWidth()` — update text freely without layout work.
- The status text is the *only* content type; for rich items compose them as
  `"Label: value"` strings.
- Like `Toolbar`, `StatusBar` overrides `setDrawFunction()` (it is a no-op):
  add items instead.

---

## Recipe 7: Selectable lists

`SelectableList` is a thin table wrapper. Each row takes a **reference** to a
persistent `bool` that holds its selection; rows inside a list share columns:

```cpp
static std::vector<std::string> s_files = { "a.bg2", "b.bg2", "c.bg2" };
static std::vector<bool>        s_sel(s_files.size(), false);

ui::SelectableList::beginList(2);            // 1 column of selectable + 1 of buttons
for (size_t i = 0; i < s_files.size(); ++i)
{
    bool rowActive = s_sel[i];
    if (ui::SelectableList::item(s_files[i], rowActive))    // returns on click
        s_sel[i] = !s_sel[i];                               // toggle manually
    if (ui::SelectableList::itemButton("Remove"))           // per-row button
        s_files.erase(s_files.begin() + i);
}
ui::SelectableList::endList();
```

**Key points:**
- `item()` returns `true` **on click**, and writes the clicked state into the
  `selected` reference. For single-selection lists pass the same variable
  pattern `selected == i`; for multi-selection toggle manually like above.
- `beginList(columns)` is an ImGui table: with `n` selection columns, call
  `item()` once and then `itemButton()` up to `n - 1` times per row (each
  `itemButton` advances one column).
- The list draws inside the *current* window/child — put it inside a
  `BasicWidgets::beginChild()` if you need it to scroll independently.
- Real-world usage: `SubmeshSelector` builds its submesh table exactly this
  way (see [Recipe 11](#recipe-11-drawable--submesh-editing)).

---

## Recipe 8: Input widgets and the mat4 trap

`Input` covers text, numbers, vectors, colors, sliders, drags and combos. The
non-obvious members:

```cpp
// Combo with a live item list (callback form):
uint32_t selected = 0;
Input::comboBox("Material", [](std::vector<std::string>& out) {
    out = currentMaterialNames();         // rebuilt every frame
}, selected);

// Drag: (0,0) range means unclamped, ImGui convention
Input::drag("Exposure", &exposure, 0.01f);
Input::drag("Sample", &sample, 1.0f, 0, 64);   // clamped int drag

// Matrix editor: decomposes into Position / Rotation(deg) / Scale rows
Input::mat4("ModelMatrix", modelMatrix);
```

**Key points:**
- Every `Input::*` returns `true` on the frame the value changed, and modifies
  the argument **in place** — pass pointers to persistent storage, not
  temporaries you discard (the edit is lost).
- `sliderDouble()` reinterprets the `double*` as `float*` internally (ImGui
  has no double slider): fine for range-limited values, do not rely on full
  double precision while dragging.
- `comboBox()` clamps `selected` into `[0, items.size()-1]` and shows entries
  prefixed with their index (`"1: Metal"`). Set `fitPreview = true` to size
  the combo to its preview text.
- **`mat4` keeps an internal cache keyed by `label`.** The euler angles live
  in a `static` map inside `Input::mat4()`, so:
  - two different matrices must use **different labels**, or they share state;
  - if the matrix is modified externally, the angles are re-extracted on the
    next draw (cheap and automatic);
  - the cache is never purged — do not generate unique labels per frame
    (`mat4("m" + std::to_string(i++))` will leak entries).
- `textWithHint()` requires the `value` string to be writable; the call
  reserves `maxLength` internally but the visible text is still bounded by the
  input width — keep default 200 for normal fields.
- `Input::vec2/3/4` have `int*`, `float*` and `glm::vecN&` overloads; with the
  GLM overloads the component order in the widget is X,Y,(Z,(W)).

---

## Recipe 9: Texture widgets (and deferred texture swaps)

`TextureWidgets` displays one texture slot (preview image + optional
click-to-pick). Two things make it non-trivial: it manages a **Vulkan
descriptor set**, and textures cannot be swapped mid-frame.

```cpp
ui::TextureWidgets _albedoWidget;

// Bind the texture to display (recreates the ImGui descriptor set):
_albedoWidget.setEditTexture(material->albedoTexture());

// Inside drawUI():
if (_albedoWidget.imageButton("albedoPick", 42, 42)) {
    auto path = app::FileDialog::getOpenFilePath(app::FileDialog::imageFilters);
    if (!path.empty()) {
        auto tex = engine->textureCache().get(path);      // your loader
        _albedoWidget.setDeferredTexture(tex);            // <- NOT setEditTexture
    }
}
```

The convenience picker `selectTexture()` bundles the image-button, a
file-dialog and a **Clear** button in one row:

```cpp
bool picked = _albedoWidget.selectTexture("Albedo", [&](base::Texture* tex) {
    // tex == nullptr  => the user pressed Clear
    // tex != nullptr  => owns the raw pointer; wrap it in a shared_ptr
    auto owned = std::shared_ptr<base::Texture>(tex);
    material->materialAttributes().setAlbedoTexture(owned);
    material->updateTextures();
    return material->albedoTexture();   // the render::Texture to display now
});
```

**Key points:**
- Inside a `draw()` call you are already rendering this frame: the descriptor
  set that references the old texture may be in flight. `setEditTexture()`
  calls `device().waitIdle()` when releasing — safe **outside** the frame,
  expensive mid-frame. Use `setDeferredTexture()` in the latter case: the
  swap is applied at the next `drawImage()`/`imageButton()` call.
- The `textureCallback` in `selectTexture()` takes **ownership** of the raw
  `base::Texture*` argument; never return without storing it somewhere.
- `selectTexture()` returns `true` only when a file was picked (not on
  Clear). The `label` is also the visible text unless it starts with `##`
  (then the ID is hidden and the preview stays clickable).
- `cleanup()` releases the descriptor set — call it from your
  `cleanup()`/shutdown path, otherwise the ImGui Vulkan pool holds it until
  context destruction.

---

## Recipe 10: Material editor driven by a SelectionManager

`MaterialEditor` has two operating modes; mixing them silently ignores manual
calls.

**Manual mode** — you tell it what to edit:

```cpp
ui::MaterialEditor matEditor;
matEditor.setEditMaterial(mainMaterial);         // replace the edit list
matEditor.addEditMaterial(otherMaterial);        // multi-edit the same values
...
matEditor.clearMaterial();
```

**Selection-driven mode** — give it a `manipulation::SelectionManager` once;
the manual setters become no-ops and the editor tracks 3D picks:

```cpp
matEditor.setSelectionManager(_selectionManager);   // install onSelect() hook
matEditor.onChanged([this]() { _document->setDirty(true); });

void drawUI() override
{
    if (matEditor.draw()) { /* a value changed this frame */ }
}
```

In selection-driven mode, every `onSelect` event rebuilds the edit list:
the **first** selected item owning a `Drawable` defines the main material
(displayed values), and all submesh materials **of that same drawable** are
edited in lockstep — moving a slider writes the value into each material of
the list and calls `onChanged` per material.

**Key points:**
- The selection callback lambda captures `[&]` in `setSelectionManager()`;
  destroy/clear the editor (or the selection manager) before the captured
  objects die — tie both to the same owning object (the delegate).
- The widget only edits `MaterialAttributes` + texture slots; after changing
  textures the editor calls `mat->updateTextures()` for you, but if you bind
  materials outside the editor, remember to call it yourself.
- Call `cleanup()` from the delegate `cleanup()` to release the six texture
  descriptor sets (`MaterialEditor` owns one `TextureWidgets` per map).

---

## Recipe 11: Drawable / submesh editing

`DrawableEditor` = `SubmeshSelector` + name/group/visibility editing for the
first drawable found in a `SelectionManager`:

```cpp
ui::DrawableEditor drawableEditor;
drawableEditor.init(_selectionManager);
drawableEditor.onChanged([this]() { _stage->document()->setDirty(true); });

void drawUI() override
{
    _leftPanel.draw();   // whose draw function calls drawableEditor.draw();
}
```

Direct access to the submesh selection is available at any time:

```cpp
auto drawable = drawableEditor.submeshSelector().editDrawable();
int32_t first = drawableEditor.selectedItem();          // -1 when none
std::vector<uint32_t> all = drawableEditor.selectedItems();
drawableEditor.submeshSelector().addSelectedItem(2);    // programmatic select
```

**Key points:**
- The "first" item is the first non-expired drawable in the selection manager;
  submesh rows *toggle* selection, so clicking two rows edits both (the
  properties panel then shows "(N items selected)" and applies name/visibility
  to all of them — the submesh **name** field only edits the first).
- A "Clear Selection" button deselects everything.
- `selectedItem()`/`selectedItems()` reflect the manager state each frame —
  query them in your callbacks, not from cached copies.
- `DrawableEditor::init()` installs an `onSelect` hook capturing `[&]` — same
  lifetime rule as `MaterialEditor` (Recipe 10).

---

## Recipe 12: Scene tree selection

`SceneTree` renders `scene::Node` hierarchies. Selection behavior is entirely
delegated to a `manipulation::SelectionManager`:

```cpp
ui::SceneTree sceneTree;
sceneTree.setRootNode(scene->rootNode());
sceneTree.setSelectionManager(_selectionManager.get());   // optional!

void drawUI() override
{
    _treeWindow.draw();   // whose draw function calls sceneTree.draw();
}
```

Click rules:

| Click | Without SelectionManager | With SelectionManager |
|-------|--------------------------|-----------------------|
| Left click | nothing (read-only tree) | replaces the selection with the node |
| Ctrl + left click (multi-selection enabled) | nothing | toggles the node in the selection |

**Key points:**
- Without a manager the widget is **display only** — no rows highlighted, no
  side effects. This is useful for read-only hierarchy previews.
- Additive mode requires `SelectionManager::multiSelection()` to be enabled;
  otherwise Ctrl+click behaves as a plain select.
- The tree reads `SelectionManager::isSelected(node)` per row every frame —
  selection made in the 3D view (pick) reflects in the tree automatically,
  no signals needed.
- Nodes with an empty name display as `(unnamed)`.

---

## Recipe 13: Reflection-driven inspectors

`ReflectionWidget` turns `bg2e::reflection` metadata into a form at runtime;
`ComponentInspector` composes it for every component of a node. This is how a
generic inspector works:

```cpp
ui::ComponentInspector inspector;
inspector.setNode(selectedNode);
inspector.onChanged([this]() { _document->setDirty(true); });

void drawUI() override
{
    _rightPanel.draw();   // whose draw function calls inspector.draw();
}
```

For non-component types (any reflected object addressed as `void*`):

```cpp
if (auto* info = reflection::TypeRegistry::get().type("bg2e::base::Light"))
{
    if (ui::ReflectionWidget::drawProperties(&light, *info)) {
        // at least one property changed this frame
    }
    ui::ReflectionWidget::drawActions(&light, *info);
}
```

**Key points:**
- Properties are grouped into collapsible trees by `metadata.category`
  (first-appearance order); widgets are chosen from `PropertyEditor` +
  `PropertyType` + `min/max/step` (see
  [reflection docs](../reflection/index.md)).
- Read-only properties (getter-only, or object without mutable getter) render
  **disabled**, not hidden — the value stays visible.
- Current editor limits to know about:
  - `Enum` properties draw a `<enum not supported>` placeholder (the concrete
    enum type is erased in `std::any`; planned to be revived with the
    registry-keyed enum metadata).
  - `Path` properties are **read-only labels** (no file dialog).
  - `Resource` / unknown types draw `<not supported>`.
- Object properties recurse as nested trees, capped at
  `reflection::maxObjectDepth`; beyond it (or for unregistered sub-types) a
  fallback label is shown.
- `ComponentInspector` lists components even without reflection data (with a
  "No reflection data" note) and can remove them; the removal is deferred
  until after the component iteration, so it is safe to remove during
  `draw()`. The "Add Component" button is an unimplemented extension point.

---

## Recipe 14: Modal loader during async work

`Loader` is a thread-safe message/progress overlay, designed for
`app::MainLoop::asyncLoad()`:

```cpp
MainLoop::current()->asyncLoad([](ui::Loader* loader) {
    loader->setMessage("Baking irradiance map...");   // safe from the worker thread
    for (float p = 0.f; p <= 1.f; p += 0.05f) {
        doWorkSlice();
        loader->setProgress(p);
    }
}, glm::vec4{0.1f, 0.1f, 0.1f, 1.f});                 // scene clear color while paused
```

**Key points:**
- `asyncLoad()` pauses the scene (solid clear color), installs the loader as
  `UserInterface` **frame override** (the delegate's `drawUI()` is bypassed),
  and spawns a detached worker thread.
- `setMessage`/`setProgress` are mutex-guarded — call them from any thread.
  `draw()` itself must run on the main thread only (the override guarantees
  that).
- When the callable returns, the queue drains on the main thread, the override
  is cleared and the scene resumes. **Never touch engine/scene objects inside
  the callable** — schedule through `MainLoop::safeUpdateScene()` or the main
  thread queue instead.
- `Loader` can also be used standalone: call `loader.draw()` manually once per
  frame from `drawUI()` (or your own `setFrameOverride`) to show your own
  progress overlay.
- The window is fixed-size (420x90 logical px), centered, non-closable and
  non-interactive: it is an indicator, not a dialog.

---

## Common pitfalls

| Symptom | Cause | Fix |
|---------|-------|-----|
| "Dear ImGui Demo" window appears unexpectedly | Base `UserInterfaceDelegate::drawUI()` draws `DemoWindow` | Override `drawUI()` in your delegate |
| Panel/toolbar never appears in a Workspace | `Window` has no draw function (Mode 2) | Call `setDrawFunction()` (or use Toolbar/StatusBar with their own widgets) |
| Workspace panel sizes ignored | `Window::options`/position set manually | Workspace overwrites them — configure `leftPanelSize()` etc. instead |
| Two widgets in the same group interfere (wrong state, tooltips on the wrong item) | Duplicate ImGui IDs | Use `##unique` suffixes or `BasicWidgets::pushId()/popId()` |
| Window position "snaps back" | `noMove`/`noResize` set | With those flags the engine re-applies position/size every frame; that is the lock mechanism |
| `setEditMaterial()` does nothing | A `SelectionManager` is installed on the editor | Selection mode wins: `clear` the manager or use a second editor |
| Texture preview shows the wrong image after a pick inside `draw()` | `setEditTexture()` called mid-frame | Use `setDeferredTexture()` inside draw code paths |
| UI freezes a frame when switching textures outside draw | `clearDS()` → `waitIdle()` | Normal; batch texture swaps to non-render times |
| Menu shortcut never fires | Item added after the first `Menu::draw()` | Build the full `MenuItem` tree before the first draw |
| Scene input reacts while dragging sliders | Input delegate is not filtered by ImGui | Ignore scene input while UI has capture (check your input delegate state; see `doc/input_delegate.md`) |
| Reflection widget shows `<enum not supported>` | v1 type-erasure limitation | Edit that property with a hand-written `Input::comboBox()` or register enum metadata (see reflection docs) |
| Transform rotation jitters/resets while editing | Matrix rewritten by code outside the widget between draws | The euler caches re-sync on external change (by design); avoid fighting the gizmo and the panel simultaneously |
| Loader progress never updates | Worker thread never calls `setProgress` | Update it; also check you passed a non-empty callable |
| Crash in `~MainLoop` / shutdown | Widgets cleaned up after `ImGui_ImplVulkan_Shutdown` | Call editors' `cleanup()` from the delegate `cleanup()` (before engine teardown) |

**Design reminders:**
- Widgets return `true` on change — drive your dirty flags from those return
  values or from `onChanged()` callbacks, not by polling values.
- The engine UI is a *wrapper*, not a framework: when a widget does not exist,
  drop to the equivalent pattern in the per-class docs, or add a `Window`
  subclass (see [Window](Window.md)).
- `bg2e::ui` is not usable from the experimental `bg2e::gpu` stack: it binds
  to `render::Engine` (Vulkan) resources.
