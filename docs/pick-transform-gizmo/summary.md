# Transform Gizmo Manipulation — Context for the math implementation plan

## Purpose of this document

The picking and input-capture plumbing for the transform gizmo is already in
place. What is **not** implemented yet is the *math*: turning a mouse drag on a
gizmo handle into an edit of the node's `TransformComponent` (translate along an
axis, rotate around an axis, scale on an axis, scale uniformly).

This document captures the relevant current state of the codebase so the next
plan can focus only on the transformation math and where to plug it in. It does
**not** propose the algorithm; it describes the foundation and the open
decisions.

## Scope of the next plan

Implement, for the four handle families, the screen-drag → transform-delta math
and apply it to the selected node's `TransformComponent`:

- **Translate** along world X / Y / Z.
- **Rotate** around world X / Y / Z.
- **Scale** along a single axis (X / Y / Z) and **uniform** scale on all axes.

The actual production rendering/manipulation will be re-done on top of
`bg2e::gpu` in the future; the current code is a fast prototype, so favour the
simplest correct approach.

## What already exists (the foundation)

### 1. Input-capture flow (`manipulation::SelectionManager`)

The app routes every mouse event through the SelectionManager first
(`apps/bg2e_composer/src/AppDelegate.cpp`). The three entry points return
`true` if the event should still propagate to the scene graph (camera
controllers) and `false` if the manager captured it:

```cpp
bool mouseButtonDown(scene::Scene* scene, int button, int x, int y);
bool mouseMove(scene::Scene* scene, int x, int y);
bool mouseButtonUp(scene::Scene* scene, int button, int x, int y);
```

Current behaviour (`lib/src/bg2e/manipulation/SelectionManager.cpp`):

- **Down**: if `transformManipulationEnabled()` and a transform gizmo is visible
  (`GizmoComponent::currentTransformNode() != nullptr`), it does a pick. If the
  hit is `PickKind::TransformGizmo` it resolves the handle from the submesh name
  and calls `gizmo->beginTransform(handle)`, sets `_capturing = true`, and
  returns `false` (camera blocked). Otherwise returns `true`.
- **Move**: returns `!_capturing`. **This is the hook for the drag math** — while
  capturing it currently only blocks propagation and does nothing else.
- **Up**: if capturing, calls `gizmo->endTransform()`, clears `_capturing`,
  returns `false`. Otherwise performs the click selection (only if the press did
  not move) and returns `true`.

Relevant state already present: `_capturing`, `_mouseDownX`, `_mouseDownY`,
`_transformManipulationEnabled`. Also `sceneInputBlocked()` and
`setTransformManipulationEnabled(bool)`.

> Note: `mouseMove(scene, x, y)` currently ignores `scene`, `x`, `y`. The math
> plan will use them (camera + cursor) and a captured drag-start state.

### 2. `GizmoComponent` interaction API

`lib/include/bg2e/manipulation/GizmoComponent.hpp`:

```cpp
enum class TransformHandle {
    None,
    TranslateX, TranslateY, TranslateZ,
    RotateX, RotateY, RotateZ,
    ScaleX, ScaleY, ScaleZ,
    ScaleUniform
};

static TransformHandle handleForSubmesh(const std::string& name, const std::string& groupName);

void beginTransform(TransformHandle handle);   // currently only stores _activeHandle
void endTransform();                            // currently only resets _activeHandle
TransformHandle activeHandle() const;
bool isTransforming() const;
```

`beginTransform`/`endTransform` are intentionally minimal right now. **The math
plan should add an `updateTransform(...)` (or equivalent) and the drag-start
state**, and have `SelectionManager::mouseMove` call it while capturing.

### 3. Axis / handle convention

From `docs` / the asset convention (`transformGizmo.bg2`): submesh suffix
`A = X`, `B = Z`, `C = Y`. `handleForSubmesh()` already encodes this:

| Submesh | Handle | World axis |
|---|---|---|
| `translateA` / `rotateA` / `scaleA` | Translate/Rotate/Scale **X** | +X (red) |
| `translateC` / `rotateC` / `scaleC` | Translate/Rotate/Scale **Y** | +Y (green) |
| `translateB` / `rotateB` / `scaleB` | Translate/Rotate/Scale **Z** | +Z (blue) |
| `scaleUniform` | ScaleUniform | all |

### 4. How the gizmo is positioned and oriented (`renderTransform`)

`GizmoComponent::renderTransform(worldTransform, view, proj, type)`:

- Keeps the **world translation** of the node (`worldTransform[3]`).
- Sets the three basis vectors to the **normalised world axes** of
  `worldTransform`, scaled by a screen-constant factor (so the gizmo keeps a
  fixed on-screen size regardless of distance).

Consequences for the math:

- The drawn handle axes correspond to the node's **world-space orientation**
  (parent rotation included), not to raw local axes.
- A drag therefore produces a delta most naturally expressed in **world space**;
  applying it to the node's local `TransformComponent::matrix()` requires
  accounting for the parent transform.

The world transform fed to `renderTransform` is the accumulated `_currentTransform`
built during scene traversal (parent chain × node transform), both in the visual
renderer (`GizmoAndSelectionRenderer`) and the pick visitor
(`PickSelectionVisitor`).

## Data available for the math

### Camera / projection / viewport

From the `scene::Scene*` passed to the mouse handlers:

- `scene->mainCamera()` → `scene::CameraComponent`:
  - `projectionMatrix()`
  - `projection()->viewport()` (logical viewport — see caveat below)
  - `ownerNode()->invertedWorldMatrix()` → view matrix
  - `ownerNode()->worldPosition()` → camera world position

Cursor `(x, y)` are in the same viewport space the picker uses. This is enough to
build a world-space ray from the cursor (unproject), which is the usual basis for
translate/rotate/scale-on-axis math.

### `TransformComponent` (`lib/include/bg2e/scene/TransformComponent.hpp`)

```cpp
const glm::mat4& matrix() const;      // local matrix
void setMatrix(const glm::mat4&);
glm::mat4 worldMatrix();              // accumulated world matrix
glm::mat4 invertedWorldMatrix();
```

Also convenience builders (`makeTranslated`, `setRotation`, `scale`, …) but the
gizmo edit will most likely operate on the raw 4×4 (`matrix()` / `setMatrix()`).

## Where the math plugs in (the gap to fill)

```
SelectionManager::mouseButtonDown  → gizmo->beginTransform(handle)   [exists]
SelectionManager::mouseMove        → gizmo->updateTransform(...)      [TO ADD]
SelectionManager::mouseButtonUp    → gizmo->endTransform()            [exists]
```

The drag math lives between `beginTransform` and `endTransform`. The
SelectionManager already owns the cursor stream and the camera (`scene`), and
already knows the current transform node via
`GizmoComponent::currentTransformNode()`.

## Suggested state to capture at `beginTransform` (decision for the plan)

To make drags absolute (delta from the press point, not incremental) and robust,
the plan will likely capture at `beginTransform`:

- the active `TransformHandle`;
- the node's **initial local matrix** (`TransformComponent::matrix()`);
- the node's **world transform** at drag start (origin + world axes used by the
  gizmo), and the **parent world matrix** (to convert a world-space delta back to
  local);
- the **initial cursor ray / position** and camera matrices.

`updateTransform` would then recompute the transform from the initial state +
current cursor, and `setMatrix` the result; `endTransform` finalises (and could
push an undo entry if/when undo exists).

## Open design decisions for the plan

1. **Local vs world application**: handles are drawn along world axes; decide how
   to fold a world-space delta into the node's *local* matrix given a possibly
   non-identity parent (decompose parent rotation/scale, or operate on world
   matrix then convert back).
2. **Translate**: closest-point between the cursor ray and the axis line → signed
   distance along the axis.
3. **Rotate**: angle around the axis — screen-tangent vs. intersection with the
   plane perpendicular to the axis through the pivot (arcball-style).
4. **Scale (axis)**: map drag projected on the axis to a scale factor; **uniform**
   scale from cursor distance to the gizmo centre.
5. **Pivot**: node origin (world translation) is the natural pivot.
6. **Decompose/recompose** of `TransformComponent::matrix()` (T·R·S) vs. applying
   a delta matrix on a captured initial matrix. Possible small helpers on
   `TransformComponent`.
7. **Snapping** (grid/angle increments) — optional, likely out of scope for the
   first pass.
8. **Visual feedback**: highlight the active handle while dragging (optional).

## Files expected to change in the math plan

- `manipulation/GizmoComponent` (`.hpp`/`.cpp`): `updateTransform(...)`,
  drag-start state, the per-handle math.
- `manipulation/SelectionManager` (`.cpp`): call `updateTransform` from the
  capturing branch of `mouseMove` (pass `scene`/camera + cursor).
- `scene/TransformComponent` (possibly): helpers to decompose/recompose or to
  apply a world-space delta in local space (the user anticipated changes here).

## Caveats

- **Deferred rescaling**: `RendererDeferred` renders at `_renderExtent`, which can
  differ from the swapchain extent. Screen→world math must use the **camera
  projection + logical viewport** (the same space as the picker's `(x, y)`), not
  the render-target extent. The depth-buffer clear extent used for the gizmo
  passes is unrelated to this math.
- **Single active transform**: only one transform gizmo is visible at a time
  (`GizmoComponent::setCurrentTransform`), so there is exactly one node being
  manipulated; no multi-target handling is needed.
- **Identifier/picking is done**: the plan does **not** need to touch picking,
  identifiers, render order, or input routing — only the math and its single call
  site in `mouseMove`.
```
