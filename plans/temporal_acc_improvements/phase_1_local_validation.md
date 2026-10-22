# Phase 1: stable surface identity and local rejection

Dependency: Phase 0 implemented. Entry: P1.1. Exit: P2.1. Preserve the Phase 0 fallback and thaw behavior.

## Contract

On the supported built-in raster path, identify each visible receiver with a stable integer token. During this phase, moving receivers deliberately reject their own history; stationary matching receivers can retain theirs. Discovered background rejects the foreground's old token. Do not yet transport samples with object motion, remove the camera threshold, or relax material/light/membership resets.

Use a new draw-level `pipeline::TemporalSurfacePass` and renderer-owned correspondence state. Do not add attachments to the G-buffer, change ObjectData's public/shader layout, or introduce GPU abstraction dependencies on draw/scene.

## P1.1 — establish persistent instance records

1. Create renderer-private correspondence state keyed by `(nodeId, submeshIndex)` with retained geometry/material snapshot references, nonzero surface token, previous recorded world matrix, and current change flags.
2. Use an incrementing uint32 token allocator. Reserve 0, never reuse removed tokens, diagnose exhaustion without wrapping. Reset the allocator only with a renderer lifecycle/history reset that discards all old correspondence. Record exact reset location.
3. Reuse TemporalSignatureState's captured effective material/texture and geometry components rather than comparing only authoring wrappers. A rigid transform changes the motion flag but not the token. Effective material/mesh/classification change replaces the token. Global resets for these edits remain in force in this phase.
4. Build immutable per-frame records. Full fingerprint/cache inspection cannot publish or consume these records. Separate a prepared update snapshot from a snapshot attached to recorded scene work. Consult recordingCompletion and its wasSubmitted state before treating prior records as a valid predecessor. If chronology is unknown, mark that instance's correspondence invalid locally.
5. Do not use the current frame slot's previous occupant as the previous scene. UI-only frames and repeated preparation are not motion steps. Missing or duplicate stable identities keep the existing explicit diagnostic when correspondence is required.

## P1.2 — implement a token metadata raster pass

1. Add `lib/include/bg2e/draw/pipeline/TemporalSurfacePass.hpp`, its .cpp under lib/src, and paired `shaders/src/draw/temporal_surface/surface.vert.{glsl,metal}` and `surface.frag.{glsl,metal}`. Copy full GPL headers. Follow existing shader discovery/name conventions; no CMake changes.
2. Start with one `R32_UINT` color image (sampled + color attachment), cleared to integer zero, and a private sampled depth image matching receiver dimensions/depth convention. Check format/usage support before initialization. No blending on the integer output.
3. Reproduce the built-in GBuffer vertex positions, culling, depth comparison/order, material UV selection, albedo alpha multiplication, cutoff and discard rules. Reuse material bindings and helpers. Add dedicated per-instance temporal uniforms/bindings for token and correspondence-valid flag rather than altering shared ObjectData. Rendering coverage, including alpha-tested holes, must match GBuffer.
4. Use private depth and repeat the same receiver draw order. Do not rely on depth-equal replay of an existing depth image: equal-depth overlapping surfaces can disagree about which instance owns a pixel. Clear and write private depth using the same rules as the original receiver pass. The temporal consumer must also reject metadata whose sampled depth disagrees with its actual surface depth.
5. Current output token is zero for an instance with no valid static correspondence in this phase (new, moved, singular/unsupported). Stable instances output their nonzero token. Recovered stationary instances can begin new history on their first valid frame. Store canonical token in the CPU record even when output is zero.
6. Enable only when the renderer uses the exact supported built-in GBuffer type and supported surface conventions. Custom/deformed receiver shaders get no metadata and retain Phase 0 behavior, with an explicit status. Do not silently claim static correspondence for unknown deformation.

## P1.3 — integrate opaque and transparent receiver domains

1. Generate opaque metadata after opaque GBuffer recording and before opaque temporal consumers. Match internal receiver resolution, not output resolution or effect quality resolution.
2. The renderer rewrites GBuffer/depth for each transparent submesh. Generate metadata matching that same current receiver domain before its temporal consumers. Repeat the original semantics: private depth begins from the preserved opaque depth for the single transparent receiver, and only that transparent item is drawn. Clear metadata color outside its covered region to invalid.
3. Do not let opaque metadata describe a transparent receiver. Generate full receiver extent first; do not optimize to the transparent scissor until coverage/clearing semantics have native evidence. A layer skipped by existing region logic produces no temporal record and must not advance its history.
4. Preserve opaque metadata separately for FSR's future use; never pass the last transparent layer's data as opaque data.
5. Retain immutable token/depth image owners for every history that needs them. Do not overwrite a slot image still referenced as previous history. Use the existing TemporaryResourcePool lifetime pattern or equivalent retained images, scoped by view/layer and recording ownership. Current metadata may be shared by signals of the same receiver invocation; retain it before recording commands.

## P1.4 — extend temporal inputs and rejection

1. Introduce optional trailing metadata inputs/owners and a versioned enhanced shader interface, e.g. `draw.temporal.v2`. Keep `draw.temporal.v1` available for custom/legacy kernels. Metadata absent means the Phase 0 path, not an exception or global reset. Built-in v2 kernels support both metadata-valid and fallback invocations through explicit flags and safe dummy bindings.
2. Validate device, extent, type, integer format and sampled usage; reject supplied malformed inputs. An unsupported optional feature selects fallback at initialization rather than retrying every frame.
3. For enhanced histories retain previous token metadata. At the reprojected nearest sample require nonzero equal tokens and current metadata depth matching the current receiver. Keep existing world position/normal/camera tests; transform motion remains locally rejected in Phase 1.
4. Signal pixels and metadata pixels can have different extents. Convert each sample through normalized framebuffer UV, using texel centers, clamping only for current sampling. Out-of-bounds previous UV must reject, not clamp onto the screen edge. Add odd-size and half-resolution examples to the probe.
5. Preserve existing reflection confidence weighting, GI valid-black handling, coverage behavior and per-key counts. Do not mistake TemporalResult::frameCount for per-pixel convergence.

## P1.5 — probes, diagnostics and acceptance

Add token and local-rejection diagnostic views through existing draw/UI wrappers, with no imgui types in public headers. Do not add per-frame CPU readbacks. Source-review CPU/GLSL/Metal layouts and integer binding types explicitly.

Extend native probe sources with: translated foreground exposing a wall; nearby parallel surfaces with equal normals; alpha-cutout holes; unchanged neighbors; object insertion changing extraction IDs; two submeshes on one node; two frames in flight; abandoned scene recording; retained earlier metadata; transparent receiver over opaque depth; custom GBuffer fallback. Keep global material/light reset assertions unchanged.

Native acceptance, when authorized: static wall token and history survive moving foreground; newly revealed pixels restart; moving object's own pixels are allowed to restart every frame. Phase 0 no-global-motion-reset acceptance must pass with metadata intentionally disabled/unavailable too.

Record all formats supported/unsupported, paired backend results, source checks and unrun scenarios in progress.md. Do not claim identity tests solve indirect-light changes on a stationary receiver. Next: P2.1.

## Cost and fallback

One extra geometry raster pass per processed receiver domain, integer token storage and private depth, plus token lookups. Many transparent layers may dominate this cost. CPU storage is proportional to instances and retained frame snapshots. Estimate actual high-water images including histories; one R32_UINT image at 1080p is 8.29 MB. No new RT rays or specialized hardware. Unavailable metadata uses Phase 0 unchanged.
