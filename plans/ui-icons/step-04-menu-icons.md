# Step 4 — Menu icons

Prerequisite: step 3 accepted. Read summary/progress and inspect Menu.hpp/.cpp, including dynamic labels, enabled callbacks, checkable entries and shortcut registration.

## Work

1. Add an optional filesystem icon path to MenuItem without changing existing constructors' meanings. Add path-based low-level Menu wrappers with explicit widget identity where needed; avoid ambiguous string/path overloads. Reuse the icon manager and shared rendering helpers rather than raw native texture IDs.
2. Support icons in action/checkable rows and submenu headers where they are provided. Preserve menu-item activation, closing behavior, disabled state, selected/check marks, shortcut text, keyboard navigation and child submenu behavior.
3. Reserve an icon gutter within menu rows, separate from check-mark and shortcut areas. Keep mixed rows aligned and incorporate the gutter in width calculation. Submit one interactive menu row, then overlay its icon through public draw facilities; do not substitute an unrelated button or create a second navigation item.
4. Keep widget identity independent of icon path and stable across dynamic visible labels. Preserve existing shortcut initialization exactly once; no shortcut re-registration on cache hits or presentation changes.
5. Use theme-appropriate logical icon sizing from menu line height, keeping the path input simple. Missing files use the shared fallback and cached failure policy. Entries without an icon preserve existing functionality.

## Deliverable boundary

Menu-level API and drawing support only. No application menu migration or new artwork required. Inspect geometry and preservation of existing callback/shortcut paths; no tests or launches.

## Handoff

Record final menu overloads and gutter/check-mark approach. Report implementation and await validation. Next: [step 5](step-05-toolbar-icons.md), toolbar content modes and measurement.
