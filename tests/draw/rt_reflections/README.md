# Manual reflection validation

Include probe.hpp in an existing temporary draw test application and call `bg2e::draw::tests::runRTReflectionsProbe(engine, shaderRoot)` with a stopped initialized Engine and deployed shaders. No target or CMake file is added. Compile engine and both native shader trees first. A false result means required native tracing/table/HDR-readback capabilities are unavailable, not a passing transport test. No agent execution is claimed.

Run with Vulkan and Metal diagnostics. The fixture checks dense/canonical IDs, geometry reuse, nonzero submesh offsets, UV1 cutouts, deterministic red-emitter mirror HDR, UV0 holes exposing a blue rear emitter, distance miss fallback, partial glossy hit confidence with unpremultiplied RGB, roughness rejection, deterministic mirror noise replacement, camera/explicit receiver agreement, display-relative output independent of receiver extent, subclass dispatch and disabled/empty neutral output.

Additional lead coverage:

- Compile all twelve surface/noise/capacity profiles, four Vulkan stages and Metal compute. Check missing RT pipeline/query/array/HDR support selects ordinary specular IBL without unsupported target/TLAS allocation.
- In existing deferred/manipulation examples combine reflections with IBL, AO and GI, with/without query shadows. Check every lighting contract/native map; Metal reflection sampling shares nearest sampler0 and uses texture16.
- Compare mirror/glossy surfaces, black hits, partial misses and roughness threshold/spread. Confirm confidence blends prefiltered IBL and applies receiver split-sum BRDF once, preserving direct lights, diffuse lighting, sheen, material AO, emission/unlit and transparent composites.
- Exercise normal-map UV0/UV1/scales under nested, nonuniform and mirrored transforms. Verify raster tangent convention, source degeneracy and fallback normal orientation. Test independent albedo/emission UV and channel/inversion/color decoding.
- Check point/spot/directional affectsReflections participation, castShadows false, source size, shadow sample counts and cutout shadow occluders. Reject nonfinite light/camera/settings and invalid surface/material/output usage/format/device/extent/aliasing.
- Resize display only, internal scale only and reflection quality. With fixed display/quality, internal-scale/FSR changes must not change reflection dimensions. Pause scene rendering while UI updates; mutate resources only at synchronized boundaries and reset the temporal upscaler for discontinuities.
- Test custom compatible shaders/subclasses, matching-capacity shared snapshots, material/geometry reload, completed-slot reuse and cleanup before Device destruction. Discard snapshots built in abandoned recordings. Direct SoftShadowPass now uses expanded device-clamped arrays too; test more than eight pairs with scene shadows enabled.

Reflection history/filtering and visible controls remain later steps. Report actual hardware/backend and scenarios validated.
