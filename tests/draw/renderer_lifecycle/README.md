# Runtime renderer lifecycle probe

Manual M01-05 fixture; not a registered executable or a substitute for the future deferred renderer example. Include `probe.hpp` in a temporary windowed draw harness and call `bg2e::draw::tests::runRendererLifecycleProbe(engine)` with an initialized Engine on the main thread, **before** initializing any other RenderLoop or UI on that engine. It uses the real window surface/command submission and no shaders. Keep the window visible and non-minimized during the probe. It creates its own loop and cleans it before returning, leaving the Engine for the caller to clean up.

The fixture checks:

- First scene initialization and a clean presentation with no extra scene submission.
- A UI-preparation callback queues a renderer while commands are recording; the active owner remains unchanged until the following frame boundary.
- The new renderer receives the same scene root. A delayed action holding the previous generation is rejected.
- Pause retains dirty scene work through resize and renderer replacement; resume renders again.
- Explicit continuous animation keeps scene work dirty. Scene sequence numbers remain contiguous across replacement, clean UI-only presentations and pause/resume.
- Target detachment precedes renderer cleanup; final cleanup leaves no active settings target.

No agent compilation or execution was performed. Compile with the engine/example definitions and link settings. Run on available Vulkan/Metal window surfaces with validation enabled. Additional application checks: actual UI/property callbacks using weak delegate ownership plus generation, real resize and output-format changes, minimize/restore, background throttling, failure during renderer initialize/attach/resize, and shutdown with queued replacement. Existing draw delegates must retain resize notifications and per-presentation frameNumber/deltaSeconds behavior. M01-15 adds the actual RendererDeferred-to-RendererDeferred example and M01-12 supplies the real settings facade.
