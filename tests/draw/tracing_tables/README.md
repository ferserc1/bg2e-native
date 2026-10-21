# Indexed tracing table probe

Status: source delivered; not compiled or executed.

Include `probe.hpp` in a native test host and call
`bg2e::draw::tests::runTracingTableProbe(engine)` with an initialized, stopped draw
Engine and no active frame or background producers. The function throws on a
failed invariant and returns true on success. False means that acceleration
structures or the fixture's resource-array profile (at least six slots) are
unavailable; report that backend gap rather than a pass. Follow the repository's
explicit-build authorization rule before compiling or running the host. No CMake
changes are needed to deliver this probe.

The fixture creates real geometry, buffers, textures and scene descriptors, then
inspects CPU snapshot tables, builds TLAS snapshots and compares native resource-set
identity. It does not dispatch shaders, render images or prove native shader behavior. Run the existing shadow, AO, GI, reflection and
deferred integration probes separately on Metal and Vulkan.

It verifies:

- One entry for repeated references to the same material/revision, distinct entries
  for independent equal-valued materials, and distinct captured revisions of one entity.
- Deterministic resource/material admission, nonzero geometry bases and submesh
  ranges, default transparent exclusion, and dense TLAS IDs distinct from canonical IDs.
- Capacity-one neutral pairs and per-instance fallback totals, independent shadow
  pair admission, and sampler-table overflow that must not admit an orphan image.
- Independent edits, staged geometry discarded after failed preparation, stale
  queue rejection after reload, empty scenes, and retained snapshots after cache clearing.

For visual acceptance, compare the phase 02 baseline with identical source order,
materials, effect settings, UV selection/scaling and capacities. Exercise real
acquired slots and aborted recording, image recreation within a stable wrapper,
sampler replacement, resize, scene replacement and object round trips. Table
addresses are transient and must not appear in serialized material identities.

Profile scenes with many unique resources and scenes with repeated references to
one Drawable/material. Record admitted instance count N and material/revision
count U; expect N mesh/material lookup requests and six image/sampler lookup
requests per emitted material. Compare exclusive table-construction time, uploaded
material bytes and fallback totals; do not infer a GPU speedup from CPU counters.

Phase 05 also checks warm material/shadow buffer and CPU-vector identity,
transform-only changes, scalar edits retaining native array sets, separate scene
sets, fresh-generation and fresh-array diagnostics, and rebuilt dense metadata
after membership/order changes. Native generations are immutable while retained.
Warm frames perform no material resource-admission lookups or material uploads;
scalar edits may repeat CPU admission but must not re-encode unchanged arrays.
