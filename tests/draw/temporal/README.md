# Manual temporal validation

Include probe.hpp in an existing temporary draw test application and call `bg2e::draw::tests::runTemporalProbe(engine, shaderRoot)` with a stopped initialized draw Engine. No CMake target is added. Compile engine and paired shaders first. A false result means unavailable temporal formats/kernels, not a passing accumulation test. No agent execution is claimed.

The fixture includes a CPU camera reference (jitter alone gives zero unjittered motion, previous-minus-current pixel direction) and actual scalar/HDR kernels on explicit and camera-depth receivers. It alternates two completed slots and simulates presentation gaps while scene samples remain consecutive. Numerical assertions cover independent AO/GI/reflection means, confidence-weighted unpremultiplied reflected RGB, retained earlier output, bounded rolling and frozen convergence, external generations, position/normal rejection, effect/view/layer separation, invalid/valid-black GI, eligible reflection misses versus roughness rejection, and disabled borrowed pass-through. Pause chronology is a CPU no-record check; it is not a claim of application pause validation.

Lead runtime scenarios on Vulkan and Metal:

- Run the fixture with diagnostics and non-RT devices where temporal formats are available. Compile all camera/world scalar/HDR profiles. Check unavailable shader/format fallback to raw signals.
- In existing deferred/manipulation examples test static convergence, continuous camera reprojection, camera switch/explicit cuts, same-camera teleports with invalidateHistories, depth disocclusion and normal edges. Validate native Y/jitter conventions at varied sizes and FSR scale.
- Verify resize, output-relative reflection quality, AO/GI quality, mode/enable/noise/temporal settings and FSR/upscaler replacement resets. SDK state and engine effect histories must remain separate.
- Edit transforms/topology/mesh/material/texture/light/environment at the queued safe boundary, including partial failure. Identity-changing edits are detected; in-place image uploads/custom shader edits require invalidateHistories. Moving objects deliberately reset all histories until object vectors are added.
- Check independent transparent layers and stable node/submesh identities. Duplicate persistent correspondence must be diagnosed when active. Test maxHistories and stale-key pruning, multiple public view IDs, sequence gaps, duplicate records and backward sequence rejection.
- Pause/retain scene color while UI/presentation continues, perform overlay/pick-only work, then resume. Only actual sceneFrameNumber samples may advance frame counts or noise. No retained image should mutate.
- Exercise asynchronous ordered submissions across slots before completion, old returned owners, reset while earlier work is in flight, abandoned recording plus invalidate, cleanup before Device destruction, custom compatible subclasses/kernels and unsupported fallback. Never call beginFrame on an unretired slot or use another queue without explicit synchronization.
- Validate confidence at partial reflective edges: valid black, misses and roughness rejection must remain distinct. Nominal frameCount is not proof of per-pixel convergence after rejection. Freeze mode must retain a saturated valid pixel while allowing rejection to restart it.

Spatial filtering and visible controls are later M03-08/09 work. Record actual hardware/backend, diagnostics and scenarios checked.
