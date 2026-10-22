# Phase 3: robust disocclusion and validated neighborhood reprojection

Dependency: Phase 2 implemented. Entry: P3.1. Exit: P4.1. Do not add adaptive ray generation, stochastic history search, motion dilation or global camera-motion resets.

## Target

Replace the enhanced nearest-history sample with a validated bilinear footprint. Reject uncovered, different-surface, occluded and inconsistent candidates separately. Preserve Phase 2 rigid-object correspondence and Phase 0 fallback. Produce confidence for later adaptive weighting.

## P3.1 — specify exact footprint sampling

1. For previous UV and history extent E calculate `q = uv * E - 0.5`, integer base=floor(q), fraction=fract(q). The four candidates are base+(0,0),(1,0),(0,1),(1,1); weights are the standard bilinear products.
2. Reject out-of-bounds taps individually. Do not clamp their coordinates and count duplicate edge samples. Reject UV/clip correspondence outside the valid projected domain before sampling.
3. Fetch each candidate's own signal, position, normal/count and corresponding token. Map the candidate's texel-center UV into retained metadata resolution; never assume receiver and signal extents match. Do not filter integer token images.
4. Reject nonfinite values, absent coverage, invalid count, unequal/zero token, failed expected previous-position/depth check, and failed expected previous-normal check before including signal values.
5. Sum valid weights W. If W <= 1e-6 reject history. Otherwise renormalize valid weights to sum 1. This prevents mixing invalid foreground/background even when only one tap survives.

## P3.2 — refine geometric checks conservatively

1. Keep existing absolute and relative thresholds as tunable floors. Add a depth-space check against expected previous view depth so a different visible surface in front of the same reprojection is rejected.
2. Define positive depth as `-previousViewPosition.z` under the current RH camera convention. Compare candidate previous position transformed by previous view with the expected previous point transformed by the same view. Both values must be finite and positive.
3. Initial depth tolerance: use the existing world-space position tolerance as the depth tolerance too; keep the full previous-world-position distance check. Do not silently loosen tests for motion magnitude. This is conservative and easy to compare with Phase 2.
4. Add optional footprint-aware tolerance only after this baseline: expose a separately named setting defaulting to zero. Calculate a bounded world-space pixel footprint from neighboring valid same-token expected-previous positions. Maximum added tolerance must be capped and documented. Never use derivatives across different tokens to enlarge tolerance.
5. For the initial normal test retain the configured cosine threshold against expected previous normal. Very thin geometry may still reject often; prefer local reconvergence over contaminating neighboring surfaces.

## P3.3 — combine signal, counts and confidence correctly

1. For scalar visibility use weighted valid scalar history. For GI retain valid-black/coverage semantics. For reflections spatially combine confidence-premultiplied RGB and confidence, then recover unpremultiplied RGB only when confidence is nonzero. Do not average unpremultiplied reflection colors indiscriminately.
2. Set conservative effective history length to the minimum count among contributing taps with nonzero weight, bounded by maxFrames; do not average long counts into a freshly restarted pixel. Current accepted sample increments that count exactly once.
3. Define geometric confidence as clamp(W,0,1) for the initial implementation. It is valid footprint support, not statistical probability. A near-zero contribution is rejected using the W threshold above.
4. Supply confidence to Phase 4 through an internal enhanced-kernel variable/structure. Do not add a permanent confidence image unless another consumer needs it. Diagnostic output allocation must be opt-in and included in memory reporting.
5. Freeze remains conditional on current valid correspondence. A disoccluded pixel restarts regardless of authored freeze, and Phase 0's active update window still overrides saturation freezing. Do not freeze a rejected or uncovered pixel because a nominal frame count is high.

## P3.4 — diagnostics and probes

Define mutually understandable diagnostic reasons: no predecessor, unsupported correspondence, off-screen/behind-camera, uncovered, token mismatch, position/depth mismatch, normal mismatch, invalid signal, accepted partial footprint, accepted full footprint. If multiple failures exist, use the first in that fixed order for debug coloring; aggregate counters may overlap if documented.

Add synthetic probe inputs for every footprint corner; half-texel and integer-center positions; 1x1/odd extents; one/two/four surviving taps; same token across a depth discontinuity; different token at same position/normal; newly uncovered black GI; reflective misses; NaN/Inf; rotated object normals; transparent edge coverage; metadata/history extent mismatch. Test exact candidate selection and weights with a CPU oracle.

Native visual scenarios when authorized: thin poles over distant wall, slowly translating alpha-cutout geometry, grazing-angle plane, moving foreground along depth, reduced-resolution effects, camera rotation, fast camera/object motion. Verify no old foreground contribution bleeds into newly uncovered wall. Remaining aliasing and stochastic noise are allowed; report them instead of arbitrarily broadening tolerances.

## P3.5 — cost review and handoff

Use existing `effect.temporal.record` timing and add opt-in diagnostic timing labels without per-pixel CPU readback. Compare nearest enhanced sampling against 2x2 at identical resolution, signal settings and scene. Disable debug outputs and signature-reference validation for timing.

Expect up to four candidate histories instead of one; caching may reduce bandwidth but do not assume a 4x or fixed millisecond runtime. No new mandatory persistent image is needed for the base algorithm. Keep only bounded early-outs; no unbounded neighborhood search.

Review GLSL/Metal parity, all no-history paths, reflection confidence math, count semantics, invalid coordinate handling and retained ownership. Record actual executed/not_run checks and pointer P4.1 in progress.md.
