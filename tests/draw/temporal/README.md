# Manual temporal validation

Include probe.hpp in an existing temporary draw test application and call `bg2e::draw::tests::runTemporalProbe(engine, shaderRoot)` with a stopped initialized draw Engine. No CMake target is added. Compile engine and paired shaders first. A false result means unavailable temporal formats/kernels, not a passing accumulation test. No agent execution is claimed.

The fixture includes a CPU camera reference (jitter alone gives zero unjittered motion, previous-minus-current pixel direction) and actual scalar/HDR kernels on explicit and camera-depth receivers. It alternates two completed slots and simulates presentation gaps while scene samples remain consecutive. Numerical assertions cover independent AO/GI/reflection means, confidence-weighted unpremultiplied reflected RGB, retained earlier output, bounded rolling and frozen convergence, external generations, position/normal rejection, effect/view/layer separation, invalid/valid-black GI, eligible reflection misses versus roughness rejection, and disabled borrowed pass-through. Pause chronology is a CPU no-record check; it is not a claim of application pause validation.

Lead runtime scenarios on Vulkan and Metal:

- Run the fixture with diagnostics and non-RT devices where temporal formats are available. Compile all camera/world scalar/HDR profiles. Check unavailable shader/format fallback to raw signals.
- In existing deferred/manipulation examples test static convergence, continuous camera reprojection, camera switch/explicit cuts, same-camera teleports with invalidateHistories, depth disocclusion and normal edges. Validate native Y/jitter conventions at varied sizes and FSR scale.
- Verify resize, output-relative reflection quality, AO/GI quality, mode/enable/noise/temporal settings and FSR/upscaler replacement resets. SDK state and engine effect histories must remain separate.
- Edit transforms/topology/mesh/material/texture/light/environment at the queued safe boundary, including partial failure. Identity-changing edits are detected; in-place image uploads/custom shader edits require invalidateHistories. Transform-only edits must preserve global history generation and FSR state. Move an object after freezing: every active history rolls during motion and for exactly 3 * maxFrames subsequent scene records, then restores the authored freeze preference.
- Check independent transparent layers and stable node/submesh identities. Duplicate persistent correspondence must be diagnosed when active. Test maxHistories and stale-key pruning, multiple public view IDs, sequence gaps, duplicate records and backward sequence rejection.
- Pause/retain scene color while UI/presentation continues, perform overlay/pick-only work, then resume. Only actual sceneFrameNumber samples may advance frame counts or noise. No retained image should mutate.
- Exercise asynchronous ordered submissions across slots before completion, old returned owners, reset while earlier work is in flight, abandoned recording plus invalidate, cleanup before Device destruction, custom compatible subclasses/kernels and unsupported fallback. Never call beginFrame on an unretired slot or use another queue without explicit synchronization.
- Validate confidence at partial reflective edges: valid black, misses and roughness rejection must remain distinct. Nominal frameCount is not proof of per-pixel convergence after rejection. Freeze mode must retain a saturated valid pixel while allowing rejection to restart it.

Phase 0 regression additions exercise the actual scalar/GI/reflection kernels at a saturated receiver, six rolling tail records with maxFrames=2 and refreezing on the seventh, motion renewal, independent view/layer keys, skipped keys/sequences, update-without-record, explicit/external resets, and unsubmitted/abandoned motion recordings. A probe subclass reads recorded CPU countdown state; it adds no production telemetry API. Boundary limits 1 and 1024 check the complete tail and final frozen pixel. The 1024 case submits 3072 one-pixel tail frames; this is a correctness fixture, not a benchmark. Shared low/high inputs limit retained input allocations in that loop. Ordinary three-argument beginFrame callers default to no motion; custom overrides must accept and forward its new motion argument.

The original no-motion freeze and immutable retained-output assertions remain. Pixel readback and immediate submissions are intentionally confined to this stopped manual fixture. None of the new scenarios has been compiled or run by the agent; unavailable formats/kernels are unsupported, never a pass.

Accepted Phase 0 limits: moving receivers may trail, and stationary receivers may retain stale AO, GI, reflected radiance and shadows as other objects move. FSR still has camera-only vectors and can trail moving objects or glass. The finite rolling window reduces stale frozen lighting without guaranteeing exact convergence. Explicit application/editor callbacks may still request global invalidation; distinguish those from renderer transform policy during interactive validation. Record actual hardware/backend, diagnostics and scenarios checked.

Optional P1.4 fixture: call `runTemporalMetadataProbe(engine, shaderRoot)` in a stopped,
initialized host after explicitly authorizing compilation/execution. It requests
`draw.temporal.v2`; false means unsupported, not pass. It checks odd receiver extent
3x3 against ceil-half-resolution signal 2x2, local token rejection, zero tokens,
current depth disagreement and previous out-of-bounds rejection. No native run has
been performed. This is separate from the legacy `runTemporalProbe` contract.

P1.5 extends the metadata fixture with actual acceptance-mask colors (including
maxFrames=1) and malformed-pair rejection. Broader raster/chronology probes and
manual acceptance instructions are in [temporal_surface/README.md](../temporal_surface/README.md).
All additions remain source-only, with no native run performed.
