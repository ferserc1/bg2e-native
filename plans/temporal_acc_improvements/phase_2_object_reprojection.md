# Phase 2: rigid-object motion and previous-surface reprojection

Dependency: Phase 1 implemented. Entry: P2.1. Exit: P3.1.

## Contract and fixed representation

Support opaque and individually processed transparent receivers with static vertex geometry transformed by invertible affine matrices. Deformations, skinning, morphing and unknown custom vertex shaders are unsupported and fall back to Phase 0 or local rejection. Do not add deformation support implicitly.

Extend the separate metadata pass to four color outputs, subject to capability checks:

| Output | Format | Meaning |
|---|---|---|
| token | R32_UINT | Canonical stable surface token; zero if correspondence invalid |
| motion | R16G16_SFLOAT | Previous minus current unjittered framebuffer pixels, internal resolution |
| previousPosition | R32G32B32A32_SFLOAT | Expected previous world position xyz; w=1 valid, 0 invalid |
| previousNormal | R16G16B16A16_SFLOAT | Expected previous world shading normal xyz; w=1 valid |

Keep private depth from Phase 1. Four attachments do not guarantee device support: check per-format usages, attachment count and combined storage limits exposed by the current API. If the existing API cannot express a required capability, document it rather than assuming support; use feature initialization failure/status fallback without modifying GPU architecture unnecessarily. A missing feature must not restore global resets on object transforms.

## P2.1 — lock coordinates and frame chronology

1. Document conventions in the API and probe: RH view, engine zero-to-one clip depth, normalized framebuffer Y opposing clip Y, motion sign previous-current, pixel units at internal extent, unjittered motion.
2. Capture current/previous unjittered and jittered camera matrices plus previous per-instance matrices in immutable scene recording state. Previous means the valid preceding rendered scene submission, not previous presentation/update/slot.
3. A submitted predecessor may be consumed before CPU completion in the existing ordered stream. An unsubmitted/abandoned predecessor from a different recording must reject correspondence. Store completion owners with snapshot data; never use a mutable live scene node as history.
4. On scene gap, cut, first use, singular/non-affine transform, invalid numbers or incompatible geometry, mark previous-surface metadata invalid locally. Keep normal explicit/lifecycle global reset behavior. Do not introduce per-frame GPU idle.

## P2.2 — generate object motion and expected previous geometry

1. Extend only the temporal pass's dedicated uniform records with previous model and camera transforms, current-to-previous normal transform, valid flag and canonical token. Match CPU alignments, GLSL std140 and Metal struct offsets with static assertions.
2. For each local vertex p compute current world `M_now*p` and previous world `M_prev*p`. Raster position uses current jittered camera. Interpolate current/previous clip/world data and perform perspective divides per fragment, not a linear interpolation of pre-divided endpoint velocities.
3. Compute pixel motion from previous and current unjittered clips using framebuffer scale `(0.5,-0.5)*internalExtent`. Reject invalid w/nonfinite results instead of pretending invalid correspondence is valid zero motion.
4. Compute expected previous world position using the same local point and previous model. For current world position x, the reference is `M_prev * inverse(M_now) * x`; use it as the CPU probe oracle.
5. Reproduce the current mapped shading normal as in GBuffer. Transform it into the expected previous world normal with `normalMatrix_prev * inverse(normalMatrix_now)` and normalize. This approximation is for the same local shading normal under rigid/affine transforms; reject singular transforms and document any unsupported reflected/nonuniform-scale cases until verified. Do not compare a rotated object's current world normal directly against its previous world normal.
6. Stop outputting token zero merely because a supported rigid transform changed. Continue to do so for invalid previous correspondence. Preserve alpha coverage and transparent domain handling.

## P2.3 — consume metadata in the accumulator

1. Keep v1 behavior unchanged. In enhanced valid v2 reprojection, locate history using the previous surface point and previous jittered view-projection; this is equivalent to applying unjittered motion plus the correct jitter delta. Use the world-point route for the first implementation to avoid ambiguous motion/jitter scaling.
2. Compare previous history position against expected PREVIOUS position, not current position. Compare previous history normal against expected PREVIOUS normal. Require matching nonzero surface tokens, previous coverage, valid clip bounds and current metadata/receiver depth agreement.
3. Reference position tolerance initially keeps existing absolute/relative settings, with the relative distance measured in previous-camera space for the expected previous point. Make the chosen formula identical in GLSL/Metal and the CPU oracle. Phase 3 refines geometric thresholds.
4. Remove the `0.001f` camera-motion whole-history rejection only for supported enhanced camera-depth inputs. Preserve projection/profile/extent/generation/sequence resets. Leave legacy/world-explicit profiles on their documented fallback behavior unless they provide a matching explicit correspondence contract.
5. Keep automatic projection-change reset conservative. Same-camera teleports/cuts must use explicit invalidateHistories/resetUpscaler; do not invent an arbitrary angular threshold. Document this requirement for callers.
6. A history first created without metadata must not suddenly be treated as metadata-valid. Switch capability/profile through local history compatibility checks. Preserve rolling/freeze countdown from Phase 0.

## P2.4 — share opaque motion with FSR

1. Add optional trailing borrowed opaque motion input and retained ownership contract to UpscalerInput; keep existing aggregate callers valid. Validate internal extent, format, device and sampled usage before passing to the SDK bridge.
2. Built-in FsrUpscaler uses supplied object-aware motion when available. Keep the existing camera-only generator as fallback. Preserve SDK sign, units, jitter and depth conventions; inspect bridge scaling instead of guessing from shader names.
3. Provide the retained opaque metadata motion generated before transparent processing, not the final transparent receiver motion. Keep the existing fullscreen camera-motion pass and extend its optional inputs to the opaque token/motion images: select object-aware motion only at a nonzero valid token, otherwise use the existing camera/depth calculation. This produces one complete motion image for the SDK, including sky/uncovered/invalid-correspondence pixels. Do not bind raw metadata motion alone, because its uncovered pixels have no object vectors.
4. Do not reset FSR on transform-only changes. Keep camera cuts, projection changes, lifecycle, settings and sequence-gap resets. The engine accumulator and SDK histories remain separate.
5. Transparent composition/refraction motion remains approximate for FSR; do not advertise it as solved or feed a single glass surface vector as motion of the entire composited color.

## P2.5 — regression and source acceptance

Extend MotionVectorGenerator CPU references or add a separate rigid-surface reference without mislabeling the camera-only API. Check stationary zero motion despite jitter; camera-only parity; known pixel translation sign; rotating plane; uniform/nonuniform scale; expected previous depth under motion along view axis; odd sizes; reduced signal extents; simultaneous camera/object motion; cuts; newly introduced instances; singular transforms; mirror transforms; aborted updates/recordings; two and three in-flight slots.

Native scenarios when authorized: a textured rigid cube translates and rotates before a wall while object history is transported; wall disocclusion restarts locally; camera pans without wholesale accumulator resets; FSR object trails improve compared with camera-only fallback. Inspect token/motion/expected-previous-position diagnostic views. Confirm fallback without enhanced formats still satisfies Phase 0.

Record source and native results separately, exact shader interface compatibility, motion units and ownership findings. Next: P3.1.

## Cost and hardware

Adds matrix/interpolator work in the metadata raster pass and per-pixel sampled data. The four metadata colors total 32 bytes/pixel before depth: token 4 + motion 4 + previousPosition 16 + previousNormal 8. At 1080p that is about 66.36 MB per complete current metadata set, not including private depth, retained/in-flight copies, or history guides. This intentionally clear first representation is bandwidth-heavy; optimization requires Phase 5 evidence. It does not add another geometry pass beyond Phase 1. No RT hardware or optical-flow dependency.
