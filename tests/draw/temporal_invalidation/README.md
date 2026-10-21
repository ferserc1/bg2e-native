# Temporal signature reference-parity probe

Status: source delivered; not compiled or executed. No CMake target is added.
Builds and native execution require explicit user authorization.

Include probe.hpp in an initialized, stopped native draw host and call
bg2e::draw::tests::runTemporalInvalidationProbe(engine, shaderRoot). False indicates
missing required color formats, not a backend pass. Paired native shaders and the
renderer initialization prerequisites are still required. The fixture executes
renderer update/preparation with exact full-signature validation; it does not
record scene rendering or verify convergence/ghosting pixels.

Checks cover unchanged slots, camera view versus projection, conservative object
motion, independent materials, below-threshold effective equality, cutoff crossing,
texture replacement, transparency, ordered membership, lights, virtual denoise
settings, explicit resets, full/cached mode switching and fingerprints queried
between an edit and the next update. Cached and full fingerprint byte formats must
match exactly, including history generation and view.

Run this and existing deferred/temporal pixel probes on Metal and Vulkan. Capture
validation decisions for environment/noise changes, source reloads, many transparent
layers and duplicate/missing stable node/submesh identities. Exercise failed or
aborted frames and source-scene destruction. Keep material/light edits globally
invalidating unless a separate narrower-reset case has native visual evidence.

Compare normal operation with setTemporalSignatureReuseEnabled(false). Use
setTemporalSignatureValidationEnabled(true) for byte/reset parity, then disable
validation before timing. Record signature payload bytes rebuilt, component hits,
full/flattened bytes, CPU timings and reset causes. Separate explicit fingerprint
queries from renderer update; vector-returning fingerprints still copy output.
Check multi-frame convergence, disocclusions and ghosting rather than accepting
single-frame equality as temporal validation. No native performance result or
narrower motion/effect-reset acceptance has been measured.
