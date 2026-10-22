# Temporal receiver diagnostics and probes

These fixtures are source only and have not been compiled or run on Vulkan or
Metal. Run only after explicit build/runtime authorization, in a stopped initialized
Engine with the paired native shader tree. No CMake changes or test auto-registration
are required by this change.

## Diagnostic controls

The existing renderer settings catalog exposes:

- `temporalSurfaceEnabled`: explicit experimental replay opt-in (still defaults off).
- `temporalDiagnosticView`: Off, Surface tokens, Local history.
- `temporalDiagnosticSignal`: Visibility, Diffuse radiance, Reflected radiance.
- `temporalDiagnosticLayer`: decimal text in the settings UI (reflection has no
  uint64 scalar); zero for opaque, otherwise the exact transparent
  `(uint64_t(objectId)<<32)|submeshIndex` layer from the current queue.
- `temporalDiagnosticsAvailable`: read-only optional kernel availability.

Choose Final or Raw HDR in the existing final-pass display settings. Raw HDR gives
unprocessed diagnostic colors. Token colors are a display hash, not a lossless token
encoding; zero tokens are black. Local history is **red for rejection/first sample,
green for accepted history, black for uncovered pixels, amber for no captured
acceptance in the selected current key** (for example disabled AO in an IBL graph).
The decision comes from the accumulation kernel, including maxFrames=1; it is not
inferred from the nominal per-key or per-pixel sample count. It does not distinguish
the individual rejection reasons. At lower signal resolution, each decision covers
the receiver pixels mapping to that signal texel.

Controls latch at scene update. Selecting a view does not implicitly enable replay
or effects. A missing/skipped layer or unsupported optional kernel leaves ordinary
scene output in place. Diagnostics bypass the temporal upscaler and use spatial
reconstruction; subsequent ordinary frames retain the upscaler's existing chronology
checks. Core accumulation continues unchanged. No ordinary-frame CPU readback,
submission or device wait is added. Queued settings imports preserve their existing
generic invalidation behavior; direct diagnostic setters do not reset histories.

## Fixtures

1. `runTemporalCorrespondenceProbe()` in `correspondence_probe.hpp` is an internal
   source fixture linked with the engine's private TemporalCorrespondence
   implementation (not an installed/public-header test). It fabricates immutable
   signatures and incomplete submission tickets. It checks insertion/extraction-ID
   shifts, two submeshes, transform-only token preservation, previous recorded world
   rather than prepared world, a submitted incomplete predecessor without waiting,
   abandoned recording, immutable earlier snapshots, reset without token recycling,
   material replacement and removal/reinsertion. Its completion ticket is a CPU
   model, not evidence of native GPU overlap.
2. `runTemporalSurfaceProbe(engine, shaderRoot, &report)` in `probe.hpp` uses actual
   RendererDeferred raster replay plus a separate scalar metadata accumulator.
   `false` means unsupported, never pass; `report` preserves initialization reasons.
   The fixture queries native format/kernel support through initialization. The
   first two commands use distinct renderer/consumer slots and are submitted in order
   before any fixture wait/readback (the GPU may finish naturally; no overlap claim).
   Later slot reuse follows explicit completion. All waits/readbacks are fixture-only.
3. Also run `runTemporalProbe`, `runTemporalMetadataProbe` and
   `runTemporalInvalidationProbe` from the existing test directories. Keep existing
   material/light/projection/membership reset expectations intact. The metadata
   fixture additionally checks exact diagnostic colors, maxFrames=1 acceptance,
   malformed pairs, odd extents, zero/local token rejection and out-of-bounds history.

The native surface fixture checks a stationary wall behind translated foreground,
nearby parallel surfaces with equal normals and depth separation below the old
position threshold, unchanged neighbors and newly revealed wall acceptance decisions,
two rasterized submeshes on one node, retained earlier pairs/diagnostic images after
slot reuse, and an abandoned scene. Its scalar signal and encoded normals are
synthetic; receiver depth and tokens are from the actual raster pass.

A nearest alpha texture creates holes. The fixture compares token coverage against
the actual GBuffer depth displayed through FinalLayer, pixel by pixel. Transparent
submeshes get independent pairs, full clears outside coverage, preserved opaque
metadata and private-depth occlusion behind the opaque wall. A derived/custom GBuffer
is explicitly unavailable. Disabling metadata leaves the Phase 0 renderer path.
The native fixture also inserts a culled object before existing extraction items,
checks changed object IDs and stable wall identity after warmup, and requires the
existing global membership reset. The CPU fixture checks identity without conflating
that reset policy with token allocation.

## Native acceptance record

For each authorized run record backend, device, shader deployment, support/fallback
status, exact assertions and visual observations in progress.md. Required formats:
R32_UINT Sampled|ColorAttachment, D32_SFLOAT private Sampled|DepthStencil|TransferDst,
R16_SFLOAT scalar history and RGBA16/RGBA32 sampled storage history, plus RGBA16
storage diagnostics. The fixture does not establish D16 support or arbitrary devices.
Currently all native formats and all runtime/visual/performance scenarios are not_run.

Separately inspect a converged wall/cube scene with real AO/GI/reflections, replay
both enabled and disabled, freeze enabled/disabled, actual application gizmos,
alpha UV0/UV1 and scaled UVs, equal-depth overlap order, skipped transparent regions,
resize, and custom fallback. Require stable wall history, local restart of newly
revealed pixels and moving receiver restart without a global transform reset.
Stable identity does not validate indirect-light changes on a stationary receiver.
No object-motion reprojection or indirect-light correctness is claimed by Phase 1.
