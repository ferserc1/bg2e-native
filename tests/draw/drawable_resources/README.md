# Independent draw resources probe

Manual M01-06 fixture, not a registered executable. Include `probe.hpp` in a temporary draw harness and call `bg2e::draw::tests::runDrawableResourceProbe(engine)` on an initialized Engine while scene/background submission is stopped, outside any active frame or command scope. Release harness-owned resources before cleaning the Engine. No shaders or renderer are needed.

The fixture loads a two-submesh standard mesh, supplies an authored three-level GPU texture, clones it and verifies:

- Distinct CPU mesh, raster vertex/index buffers, material objects, six texture images and six samplers per submesh.
- Preserved submesh names/groups/transforms and scalar texture channel/UV metadata.
- Byte-exact authored mip copying; changing original mip pixels cannot change the clone.
- Original vertex reload/material edits leave cloned CPU data and its uploaded snapshot unchanged.
- Independent built tracing geometry referencing the clone buffers where device acceleration structures are enabled; ordinary raster behavior on devices without that capability.
- Wrong-mode component access rejection, draw-aware transformed/visible bounds, and deep scene component cloning.

No agent compilation or runtime execution was performed. Compile with existing draw example settings; run on available Vulkan/Metal devices with validation enabled. Also check filesystem SDR/HDR/procedural loads and repeated reloads, source settings edits through setters, alpha/emission/normal/UV1 metadata, authored cubemap face/mip clones, preservation of mixed source layouts, and unsupported texture filtering/generation errors. Verify ordinary production examples and component bounds/clone behavior separately.

Additional cache/context checks with a valid image fixture: repeated explicit draw TextureCache loads return the same live immutable input; differing sampler/color settings produce separate entries; a second Engine/device never receives the first device's image; cloning a cached input produces independent images/samplers. Cache entries are weak and must not extend device lifetime. Treat cached inputs as immutable and release all external GPU resources before Engine teardown.

Actual raster material/texture shader routing is integrated in M01-09/10; this probe validates resource independence, not the future deferred renderer. Draw-aware DB load/save arrives in M01-07; an attempted legacy serialization of a draw component currently fails explicitly.
