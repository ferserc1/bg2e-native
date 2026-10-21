# M08-04 draw CLI verification

Opt-in fixtures only; no CMake/test target. The agent has not compiled or executed
them. The project lead owns native and executable validation.

For the CPU probe, include probe.hpp in a local driver, add
apps/lightmap_generator/src to its include directories and link that
CommandLine.cpp plus the engine. Call runDrawLightmapCliProbe with a fresh,
caller-owned scratch directory. It checks defaults, argument grammar and ranges,
mode/format constraints, independent model-copy controls, typed codec import,
CLI-over-config precedence, protected config input, strict/skip diagnostics and
incompatible renderer rejection. No Engine is initialized.

For the actual executable:

1. Configure/build the draw_lightmap_generator target on supported native hosts.
   Verify --help exits 0 without GPU initialization; malformed/missing/duplicate
   arguments and invalid configuration/path inputs exit 2, with no output writes.
2. Model BG2/VWGLB/OBJ and single/multi-root prefab jobs: AO/GI, one/several frames,
   UV2 generation on/off, explicit model copies, RGB8 and RGB32F, overwrite on/off.
   Resolve only lightmaps.json paths and check counts/mode/UV2/source preservation.
3. Import the shipped settings.example.json, override resolution/frames regardless
   of option order, and check actual metadata. Test typed numeric overflow,
   unknown/unavailable properties, type/renderer/schema mismatch and skip warnings.
   No editor/display settings should silently become bake settings.
4. Exercise Vulkan, Metal on macOS, invalid/unavailable backend/capability and
   missing shader roots. Required RT failure must exit 1 without publication.
   Existing production lightmap_generator must remain buildable and unchanged.
5. Send SIGINT/SIGTERM before work, during assembly, between targets and after
   completed samples. Cancellation returns 130/143, publishes no new manifest and
   preserves the old complete product. Interrupts after commit do not undo output.
6. Exercise unwritable destinations, protected-input aliases, output locks, failed
   native model snapshot, malformed required geometry and cleanup failure. Check
   exit 1 and correct previous-generation preservation, including documented
   unreferenced generations/stale locks after crashes or final publication errors.
7. Build from scratch and after a shader-only edit, then inspect the active bundle
   or product shader root for paired draw/bake trees, including
   bake/uv_surface/surface.vert.metallib and its .spv peer where applicable.
   Verify Distribution/RuntimeShaders packages and a relocated executable.
   Shared bundle_app/paired refresh/package discovery rules are reused unchanged.

No native/performance evidence is implied by the source-level checks. M08-05 owns
the full product matrix; use tests/bake/batch for file/model/HDR integrity details.
