# Shadow v5 binding reuse probe

Status: source delivered; not compiled or executed. Rebuild the library and both
native shadow shaders together before running integration checks. Builds and
application/test execution require explicit user authorization.

Include `probe.hpp` in a native test host and call
`bg2e::draw::tests::runShadowBindingProbe(engine)` on an initialized, stopped Engine
with no active/background frame. False reports an unavailable acceleration/array
profile; it must not be recorded as a passing backend. The probe allocates real
resources and synchronously builds TLAS snapshots. It does not dispatch the shadow
shader or compare rendered pixels.

Checks cover the v5 invocation/scene/persistent-array split, repeated stable-set identity, independent layouts
and capacities, rejection of mismatched array sizes, recovery after failed creation,
fresh-set diagnostics, native owners retained across source reload, all-opaque
tables with full fallback arrays, and stale native-resource rejection. Metal also
checks compatible encoder identity reuse, distinct argument buffers for distinct
sets, unchanged array storage, and retained indirect texture residency. Backend
array mutation checks use a private uncached diagnostic set.

Run the following integration checks on both Metal and Vulkan:

1. Render opaque geometry and several transparent layers with shadows. Compare
   normal reuse with `SoftShadowPass::setBindingReuseEnabled(false)`. Capture each
   invocation's normal/depth inputs and both visibility outputs; earlier inputs
   and outputs must remain associated with their own invocation.
2. Cover no alpha-tested materials, one cutout, multiple textures, overflow, no
   occluders, no lights and disabled shadows. Occupancy may be zero while an active
   scene's allocated alpha array capacity remains device-sized and fully initialized.
3. Change layouts, capacities, targets/resolution, frame slots and scene. Reuse only
   a matching native tuple/layout/set/capacity; keep source resources and each invocation
   alive through GPU completion. Test standalone SoftShadowPass as well as the renderer.
4. Exercise exact image subresource ranges, in-place recreation and sampler
   replacement. Invalid changes must fail before cached resources are returned.
   Abort recording, discard that snapshot, and retry with a fresh snapshot.
5. On Metal, stress concurrent encoding by distinct resource sets sharing one layout.
   Encoders serialize their mutable state; argument buffers and residency lists
   remain independent. Verify argument residency on every reused bind in a capture.

For I invocations and K distinct native tuple/layout/set/capacity combinations, expect
I invocation bindings and K alpha-array preparations, including full allocated
capacity for every preparation. Metal encoder creations scale with distinct layout
argument groups, not invocations or snapshots sharing a layout. Resolved-entry
counts exclude repeated fallback entries and unchanged retained sources. Encoded
entries include every initialized capacity slot. Capture counters and exclusive
CPU/GPU timings with the plan's scene metadata; no speedup has been measured yet.
