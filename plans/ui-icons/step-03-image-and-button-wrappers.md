# Step 3 — Image and button wrappers

Prerequisite: step 2 accepted. Read summary/progress and inspect existing Button/Layout/Text wrappers.

## API direction

Provide a small image wrapper (for example `ui::Image::icon(path, size, ...)`) and explicit Button methods such as `iconButton(id, path, size, ...)` and `iconTextButton(id, label, path, size, ...)`. Exact options/signatures should follow repository conventions and be recorded before completion. Add equivalent shared-Icon overloads. Paths use `const std::filesystem::path&`; identifiers and labels remain strings. Do not force callers to pass UserInterface or IconManager to every widget.

## Work

1. Resolve path overloads through the active session manager and delegate both overload families to shared drawing code. A cache hit performs no file access, decoding, upload or new registration. Existing text buttons keep their API and behavior.
2. Support logical icon size, neutral/default tint, optional tooltip, disabled state and existing same-line conventions where applicable. Scale logical icon dimensions once; preserve aspect ratio within the requested box. Keep transient TexturePreview dimension semantics unchanged.
3. Implement icon-only and icon-plus-text buttons as single interactive widgets with one stable explicit ID, keyboard/gamepad activation, hover/focus, disabled handling and hit rectangle. Reuse supported public ImGui button and draw-list facilities where possible; do not create separate clickable text/image items or depend on imgui_internal.h unnecessarily.
4. Separate layout measurement from submission through one shared private content-size helper. Account for text, icon box, padding and spacing consistently. This helper will support toolbar right alignment.
5. Preserve alpha, tint and disabled rendering coherently. Draw a question-mark substitute in the same allocated box when no fallback texture exists; keep button identity and layout unchanged. An empty optional path means no icon, whereas a nonempty failing path means fallback.
6. Define safe behavior outside an active UI session or with stale/cross-session Icon handles. Never dereference stale backend resources or silently attach an old handle to a different engine.

## Deliverable boundary

Reusable image and button wrappers, public includes and necessary internal helpers. No menu, toolbar or settings changes yet. Inspect public API compatibility and shared draw/measure paths; do not compile or run tests.

## Handoff

Record exact signatures, size units and invalid-session behavior. Report implementation and await validation. Next: [step 4](step-04-menu-icons.md), using the same path/resource infrastructure in menus.
