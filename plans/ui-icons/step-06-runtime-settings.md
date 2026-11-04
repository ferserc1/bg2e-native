# Step 6 — Runtime toolbar settings

Prerequisite: step 5 accepted. Read summary/progress and inspect UserInterface initialization/cleanup, UISettingsWindow, PreferencesStore and toolbar configuration.

## Work

1. Own session-wide toolbar defaults in the UI layer: presentation mode and logical icon size. Offer minimal getters/setters; do not store Toolbar pointers in the settings window or copy mutable defaults once into each toolbar.
2. Add a Toolbar section to the existing UISettingsWindow with the three presentation choices and a sensible bounded logical icon-size control. Use existing UI wrappers and English labels. Account for the existing fixed window dimensions so controls remain reachable.
3. Inheriting toolbars observe changes on their next draw without recreation, reloading textures or losing button identity. Per-toolbar explicit overrides remain effective and can be reset to inheritance. Text-only entries stay usable in all modes.
4. Persist values under the existing `ui` preferences context using stable keys such as toolbarPresentation and toolbarIconSize. Validate enum values and finite positive size on read; use documented defaults for absent or malformed data. Choose a conservative default preserving existing text presentation.
5. Load settings before delegate initialization, update state immediately on control changes and follow existing preference persistence conventions. Reset runtime state appropriately between UI sessions. Keep all icon source/cache keys unchanged when presentation, size, tint or UI scale changes.

## Deliverable boundary

Runtime and persistent settings complete. Do not add new settings windows or per-icon preference systems. Inspect initialization/persistence and draw-time inheritance paths only; the user performs execution validation.

## Handoff

Record preference keys/defaults/range and override behavior. Report implementation and await validation. Next: [step 7](step-07-documentation.md), documenting the final API and ownership contract.
