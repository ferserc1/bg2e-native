# Manual bilateral denoise validation

Include probe.hpp in an existing temporary draw test application and call `bg2e::draw::tests::runDenoiseProbe(engine, shaderRoot)` with a stopped initialized Engine and deployed shaders. No CMake target is added. Compile the engine and native profiles first. False denotes missing required profiles, not a passing filter test. No agent execution is claimed.

The fixture verifies live virtual getter/setter metadata through SettingsCatalog and an injected record override. Numerical GPU checks cover scalar noise reduction, coverage neutral values, constant HDR values above one, 5x3 odd extents/borders, world-position and normal discontinuities, reflection confidence weighting without RGB premultiplication, invalid GI centers, roughness rejection, native camera-depth profiles, a 3x1 HDR signal over 5x3 surfaces, retained old outputs, radius-zero identity and disabled borrowed pass-through.

Additional project lead checks on Vulkan and Metal:

- Compile all camera/world scalar/HDR profiles. Validate missing scalar format, HDR format or shader profile returns its original signal without wrong-format output; partial profile support remains usable. Test without RT hardware.
- In existing deferred/manipulation examples inspect raw -> temporal -> spatial -> lighting order with AO, GI, reflections and transparent layers; filtered data must never feed the temporal history or exact bake average.
- Exercise radius 0/1/8, sigma boundaries 1e-6 and 1e6, finite threshold bounds and rejection of NaN/infinity/bad accessor values. Verify HDR constant fields, black valid GI/hits, zero-confidence misses, roughness masks and geometry edges at image borders.
- Check camera linear-depth thresholds with perspective/orthographic views, jitter, nonuniform surface/signal scales, output-relative reflections, resize and FSR switches. Explicit world distance must not depend on an invented bake camera; M04 owns chart isolation policy.
- Validate invalid formats/usages/devices/material extents/view matrices, unretired or duplicate serials/keys and invocation budget overflow. Check fresh sampled/storage transitions with native diagnostics.
- Use asynchronous acquired slots and externally retained output owners; confirm later effects/views/layers cannot overwrite earlier results. Keep borrowed inputs and Device alive through completion, and cleanup at a stopped boundary.
- Supply compatible custom kernels/subclasses and metadata/settingsInstance overrides. Enumeration must read actual getters and edits must invalidate SDK/engine state through application boundaries. Visible controls/preferences registration remains M03-09.

Record actual hardware/backend, diagnostics and scenarios exercised.
