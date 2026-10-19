# Exact frame accumulation probe

Manual M05-03 fixture. Compile/link `main.cpp` against the current shared engine
using the existing project dependencies. No CMake target is added. The agent has
not compiled or executed this fixture.

Checks: exact two-sample RGB mean (1 and 3 resolve to 2), signed HDR, opaque alpha,
N=1, premature resolve, oversampling, reset, invalid dimensions/count, nonfinite
RGB rejection without partial sample commit, and RGBA16F tight readback/upload
round trip through an in-memory Image implementation. It does not verify native
transfers, shader output or GPU completion. The offscreen fixture covers the
native driver and offers the real deferred path.

Project lead checks: run this fixture, then the optional deferred case in
`../offscreen`. For an N-sample scene set `config.frames = N`, verify consecutive
completed indices, `completedFrames() == N`, and `hasResult()` only in the final
completed callback. Verify no final color pass/SMAA runs before the final sample.
Repeat with N=1, N=2, reduced scale, RT disabled, and unavailable FSR. Compare
normal RendererDeferred output at N=1; intermediate surface clears are not an
exportable result. A controlled renderer source sequence 1/3 must resolve to 2
before final color processing, not the mean of two tone-mapped values.

Import an actual UI settings export using graphicsConfig; unknown lightPose IDs
must reject unless explicitly mapped by extendSettingsCatalog or explicitly
skipped with reported settingsResult issues. Verify explicit documents bypass
persisted defaults and explicit internalExtentOverride/scaleOverride win. Invalid
JSON, target types, counts and conflicting overrides fail before any scene sample.
Mutate settings/camera/material between frozen samples and verify rejection with
no result; repeat with nonzero timestep to exercise an intentional animated mean.
GPU content edits in place must explicitly invalidate histories or start a new
job; pointer identity cannot discover such writes.
