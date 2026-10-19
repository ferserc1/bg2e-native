# Experimental manipulation example

`examples/draw/11_manipulation` registers the `draw_manipulation` target. It
demonstrates experimental draw selection, gizmo overlays, camera input routing,
partial material edits and best-view analysis. Follow the
[tutorial](../tutorials/draw/11_manipulation.md) for the interaction sequence
and API references.

The example is a development sample, not a production editor. It uses the
engine's UI wrappers and platform-neutral input API. The executable requires
the project's normal runtime resources (shaders and assets); follow the common
[draw example distribution notes](draw_deferred_renderer.md) for build,
platform-generator, SDK and packaging requirements. No Vulkan runtime
validation is claimed by this page.

## Current startup and lighting controls

This example starts at 70% internal resolution with GI selected, GI enabled and
reflections enabled. GI and reflections now match the global draw renderer defaults;
FSR/SMAA remain disabled by this example configuration.
Actual tracing depends on available/status. Graphics settings opens at startup;
the Manipulation window's Graphics settings button reopens it. The window/catalog
is explicitly owned and connected, rather than registered by MainLoop.

Expand **Manipulation > Lights > Light 1..4** for type, color, intensity, spot
angles, shadow casting, source size/sample budget, reflection participation, local
position and XYZ Euler degrees. The dedicated editor uses actual catalog descriptors
and queued edits, so it agrees with Graphics settings. Editing here pauses light
animation without resetting the Lights parent rotation. Position/orientation are
local to that parent; each edit preserves other TRS parts. Omni lights ignore
orientation; spot/directional lights use transformed local +Z. Re-enable Animate
lights if desired; pause it for temporal convergence comparisons.

Light pose targets also appear in Graphics settings and JSON/preferences through
an example-owned extension; save/restore is explicit and no preferences load
implicitly. Renderer/catalog rebinds reject old handles. Material/gizmo/light edits
reset engine and SDK histories at synchronized boundaries, including partial edit
reports. Weak selection/clone independence and separate picking pixel scale remain.

Use [live settings](../api/draw/Settings.md), [lighting architecture](../architecture/draw_advanced_lighting.md)
and the [manual validation matrix](../../tests/draw/advanced_integration/README.md).
No image/performance parity or new agent runtime execution is claimed.
