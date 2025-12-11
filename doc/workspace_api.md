# Workspace Layout System Documentation

## Overview

The **Workspace** class defines a lightweight, customizable layout system used to arrange UI windows within an application.  
It is part of the `bg2e::ui` namespace and provides support for:

- A **tool bar** (top area)
- A **left panel**
- A **right panel**
- A **bottom panel**
- A **status bar** (bottom-most area)
- A central workspace area computed automatically

This system is intentionally simpler than ImGui's docking system and uses absolute positioning and size calculations to guarantee predictable layouts.

---

## Initialization

A workspace is initialized using:

```cpp
void Workspace::setup(
    uint32_t width,
    uint32_t height,
    Window* toolBar,
    Window* leftPanel,
    Window* rightPanel,
    Window* bottomPanel,
    Window* statusBar = nullptr
);
```

Each pointer may be a valid `Window*` or `nullptr`.  
Passing `nullptr` means the panel is not used.

### Example

```cpp
_workspace.setup(
    uiWidth(), uiHeight(),
    &_toolBar,
    &_submeshPanel,
    &_environmentPanel,
    nullptr,      // no bottom panel
    &_statusBar   // status bar enabled
);
```

> Noe: The functions uiWidth() and uiHeight() are provided by the `bg2e::ui::UserInterfaceDelegate` interface. They return the current dimensions of the UI rendering area and are typically updated each frame based on the size of the application window. The Workspace system is designed to be used within a UiDelegate implementation, where these functions allow the layout to adapt automatically to window resizing. Using Workspace inside the UiDelegate ensures that all panels are positioned and sized consistently with the actual UI viewport.
>
> For more information, check the [document about application delegate system](delegate_system.md)

After setup, the layout is computed automatically.  
The workspace will adapt to window size changes via `resize()`.

---

## Resizing

```cpp
void Workspace::resize(uint32_t width, uint32_t height);
```

This function updates internal viewport dimensions and recomputes all panel positions and sizes.

---

## Drawing

```cpp
void Workspace::draw();
```

This renders all visible panels.  
Each panel must have its own draw lambda already configured in its `Window` instance.

---

## Panel Visibility Controls

Each panel can be shown or hidden dynamically:

```cpp
workspace.showLeftPanel();
workspace.hideLeftPanel();
workspace.toggleLeftPanel();
workspace.setLeftPanelVisible(true_or_false);
```

Equivalent methods are available for:

- Tool bar
- Left panel
- Right panel
- Bottom panel
- Status bar

Whenever visibility is changed, `updateWindows()` is invoked automatically.

---

## Panel Size Configuration

The workspace provides a `PanelSize` struct for left, right, and bottom panels:

```cpp
struct PanelSize {
    uint32_t min = 200;   // Minimum size in pixels
    uint32_t max = 400;   // Maximum size in pixels
    float    relative = 0.2f; // Relative size of viewport
};
```

The final size is computed as:

```
clamp( viewportSize * relative, min, max )
```

### Accessors

```cpp
workspace.leftPanelSize().min = 150;
workspace.leftPanelSize().max = 450;
workspace.leftPanelSize().relative = 0.18f;
```

Examples:

```cpp
workspace.bottomPanelSize().min = 100;
workspace.bottomPanelSize().relative = 0.25f;
```

These values can be changed **before or after** `setup()`.  
Any change applies on the next `updateWindows()` call.

---

## Layout Rules

The workspace arranges panels in this order (from top to bottom):

1. **Tool Bar**
2. **Main Workspace Area**  
   - Left panel  
   - Right panel  
   - Central area between them
3. **Bottom Panel**
4. **Status Bar** (always at the bottom of the window)

### Vertical padding

The vertical layout uses three paddings:

```cpp
topPadding      = toolBarHeight (if visible)
bottomPadding   = bottomPanelHeight (if visible)
statusPadding   = statusBarHeight (if visible)
```

Available vertical space for the central content:

```
viewportHeight - topPadding - bottomPadding - statusPadding
```

### Horizontal layout

- Left panel: fixed width (based on PanelSize)
- Right panel: fixed width (based on PanelSize)
- Central area: remaining width

---

## Example with Custom Configuration

```cpp
// Configure panel sizes before setup
_workspace.leftPanelSize().min = 120;
_workspace.leftPanelSize().max = 300;
_workspace.leftPanelSize().relative = 0.2f;

_workspace.rightPanelSize().relative = 0.25f;
_workspace.bottomPanelSize().min = 150;

// Initialize workspace
_workspace.setup(
    uiWidth(),
    uiHeight(),
    &_toolBar,
    &_submeshPanel,
    &_environmentPanel,
    &_logPanel,
    &_statusBar
);

// Dynamically hide the right panel
_workspace.hideRightPanel();

// Restore it later
_workspace.showRightPanel();
```

---

## Notes

- All panels are optional.
- If a panel is `nullptr`, it is treated as "not present".
- Visibility flags allow enabling/disabling panels at runtime.
- Internal layout never exposes ImGui directly; user code interacts only through `Window`.

---

## Summary

The Workspace API offers:

- A deterministic and stable non-docking UI layout
- Simple control over visibility and sizing
- Automatic recomputation on resize
- Configurable minimum, maximum, and relative panel sizes
- A clear vertical stacking order that includes a new **status bar**

It is ideal for editors, tools, and any workflow requiring predictable UI zones without the complexity of ImGui's docking system.

