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
