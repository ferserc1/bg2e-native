# Manual RTGI validation

No test target or CMake file is added. Include probe.hpp in an existing temporary draw test application and call `bg2e::draw::tests::runRTGIProbe(engine, shaderRoot)` with a stopped initialized Engine. Compile the engine and all paired shader stages first. No agent execution is claimed. A false result means required native tracing/material-array/HDR-readback capabilities are unavailable, not a passing transport test.

Run on Vulkan and Metal with native validation. The fixture checks retained geometry/dense IDs/nonzero submesh bases and UV1, red emission above an identical nonemissive baseline, UV0 cutouts exposing a blue rear emitter, distance exclusion versus explicitly excluded geometry, environment escape, shared blue noise and multiple bounces, participating point-light contribution, camera/explicit surface agreement, virtual dispatch and zero-alpha disabled/empty output.

Lead integration coverage:

- Compile all camera/world, hash/blue and 32/128/256 profiles. Vulkan must load all four RT stages with recursion depth one; Metal must use compute queries. Record unsupported profiles separately.
- In existing deferred/manipulation examples, switch IBL/AO/GI, toggle enabled independently, resize/change quality, pause/update UI, edit/reload textures/geometry/materials, and verify requested settings survive unsupported/disabled fallback.
- Check geometric normals under nested nonuniform/mirrored transforms, both UV sets and scalar emission channel/inversion/sRGB metadata. Use emissive geometry with a nonzero submesh offset and transparent/cutout occluders.
- Test point/spot/directional light participation, shadow toggles, source size and shadow sample counts, with an actual rear shadow occluder and a hole revealing it. Inspect black valid GI versus invalid zero-alpha fallback.
- Verify diffuse IBL replacement without double ambient, retained specular IBL/sheen/material AO/baked multiplier, direct lights/emission/unlit and transparent composites. No baked presence policy exists until M04.
- Exercise independent image/sampler table limits and overflow, compatible shared snapshots, source reload lifetimes, completed-slot reuse, custom compatible shaders/subclasses and cleanup before Device destruction. Disable legacy SoftShadowPass for material tables above its separate eight-cutout-pair limit.
- Check no-RT/no-native-pipeline/no-array capability fallback, invalid output/surface formats/usages/devices/extents/aliases, duplicate preparation serials and invalid light/settings values. No unsupported GI target/TLAS should be created by RendererDeferred.

History, filtering and visible settings controls remain later steps. Report actual hardware/backend and scenarios exercised.
