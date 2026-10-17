# Manual RTAO validation

This fixture is not wired into CMake. Include probe.hpp in a temporary existing draw test application and call `bg2e::draw::tests::runRTAOProbe(engine, shaderRoot)` with an initialized draw Engine and the deployed shader root. Compile the engine and both native shader profiles first. No agent execution is claimed.

Run on Vulkan and Metal with native diagnostics. A false result means the query/table/scalar capability profile is unavailable; record that separately from a passing AO run. The fixture checks explicit surfaces, dense versus canonical instance IDs, nonzero submesh offsets, geometry reuse, UV1 alpha masking, UV0 holes revealing a rear occluder, radius exclusion, tracing visibility, shared BlueNoise, disabled neutral output, subclass dispatch and camera/explicit surface equivalence. It reads the raw scalar output with numerical tolerances. Depth is cleared through a rendering attachment, not uploaded.

Additional lead checks:

- Run the existing deferred and manipulation examples with AO disabled/enabled; preserve direct lights, emission, unlit materials, material AO, environment lighting and transparent-layer composition. Inspect diagnostic occlusion output.
- Change all four quality levels, renderer resolution, viewport and scene attachment. Check output extents, no validation errors, no stale descriptors and correct cleanup while a shared pass is externally retained.
- Check no-RT/no-array capability fallback without AO target/TLAS allocation. Check explicit old lighting shader contracts and a compatible custom AO subclass/shader prefix.
- Exercise independent image/sampler deduplication, 32/128/256 profiles where supported, material texture reload, drawable reload, transform/mirror changes and table overflow diagnostics. Disable legacy ray-query soft shadows for scenes exceeding its separate eight-cutout-pair limit.
- Reject wrong formats/usages/devices, mismatched extents, missing explicit coverage, duplicate preparation serials, output aliases, radius <= 2*bias and invalid settings. Verify empty and disabled output is one without receiver reads.
- Retain a snapshot across later preparation and source reload; reuse only completed slots. Discard snapshots if a command recording that built them is abandoned. Clean up before destroying the Device.

AO history, filtering, visible settings controls and baked-light presence policy remain later plan steps. Report hardware/backend, diagnostics and scenarios actually exercised.
