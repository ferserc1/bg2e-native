# Step 5 — Toolbar icons and presentation

Prerequisite: step 4 accepted. Read summary/progress and inspect Toolbar, Window, Workspace and existing ToolbarButton initializers.

## Work

1. Append optional icon path and tooltip fields to ToolbarButton while preserving existing fields and aggregate initializer order. Do not require applications to load Icon resources. Label remains available as tooltip/accessibility text in icon-only mode.
2. Add toolbar presentation values Text, IconsWithTooltips and IconsAndText, plus logical icon size in toolbar configuration. Provide runtime setters/accessors and an inherit-session-default setting so step 6 can drive all ordinary toolbars. Keep configuration independent of individual image pixel dimensions.
3. Keep existing text-only applications usable: buttons without an icon remain text in every mode. Nonempty missing paths show the shared fallback in icon modes. Explicit tooltip takes priority; icon-only buttons otherwise use label. Preserve actionless entries as noninteractive content with matching presentation rules.
4. Use the shared button renderer and shared content measurements for left/right groups, including icon dimensions, text, padding and spacing. Use floating-point sizes internally. Align both groups vertically and recompute widths each draw so mode, scale and labels can change live.
5. Scope widget identity by toolbar instance and button ID, never by path or visible label. Inspect the existing findButton implementation and correct its cross-container iterator comparison as part of reliable icon-enabled button lookup. Enforce/document the existing unique-ID promise for explicit duplicate IDs without silently producing widget-ID collisions.
6. Ensure add/update/enable/disable operations and runtime path changes behave consistently. New requested paths stay cached once used; changing a button does not evict earlier icon resources. Preserve menu integration and avoid unrelated Toolbar cleanup.

## Deliverable boundary

Toolbar API and rendering work with programmatic mode/size changes. UI settings binding follows next. Do not add placeholder paths to application toolbars or generate application artwork. Inspect old aggregate call sites and draw/measure consistency only.

## Handoff

Record defaults, ID policy, inheritance and fallback rules. Report implementation and await validation. Next: [step 6](step-06-runtime-settings.md), exposing presentation in UISettingsWindow and persisting preferences.
