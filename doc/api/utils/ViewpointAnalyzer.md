# ViewpointAnalyzer

**Header:** `<bg2e/utils/ViewpointAnalyzer.hpp>`  
**Namespace:** `bg2e::utils`

`ViewpointAnalyzer` is a draw/GPU-backed utility for scoring
camera candidates from scene geometry. It borrows a `draw::Engine*`; call
`init()` before analysis and `cleanup()` before engine teardown.

```cpp
utils::ViewpointAnalyzer analyzer(engine);
analyzer.init();
const auto samples = analyzer.analyze(scene);
const auto* best = utils::ViewpointAnalyzer::bestSample(samples);
if (best)
{
    utils::applyViewpointToOrbitCamera(best->camera, orbit, camera);
}
```

Analysis renders candidate views to RGBA8 identifier and D32 depth targets and
reads them back synchronously. Stop scene and GPU producers while `analyze()` runs;
it is an explicit action, not per-frame work. `analyzedSubmeshes()` maps
analysis identifiers back to weak scene node/drawable references. These
identifiers are local to the analysis snapshot.

`ViewpointSamplingConfig` controls target size and sampled yaw, pitch, target
height, distance scale and field of view. `ViewAnalysis` reports visible
pixels/submeshes and depth distribution; `ViewSample::score` is the analyzer's
ranking value. Empty or non-analyzable scenes may yield no best candidate.

The draw overload of `applyViewpointToOrbitCamera()` updates orbit parameters
for the selected candidate while preserving the camera's existing projection
and its FOV/near/far settings. Candidate projection values are used only by the
analysis render. See
[`SceneCameraUtils`](SceneCameraUtils.md) and the
[manipulation tutorial](../../tutorials/draw/11_manipulation.md).
