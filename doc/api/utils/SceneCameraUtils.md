# Scene camera utilities

**Header:** `<bg2e/utils/SceneCameraUtils.hpp>`  
**Namespace:** `bg2e::utils`

`cameraVerticalFieldOfView(camera, fallback)` returns a camera's vertical FOV
in degrees, or the fallback when the camera/projection does not provide one.

`applyViewpointToOrbitCamera(candidate, orbit, camera)` accepts
`utils::CameraCandidate` and updates orbit center, distance and rotation while
preserving the target camera's projection and its parameters. Candidate FOV and
near/far values describe only the analysis render; applying a candidate never
copies them into the scene camera.

The camera and orbit controller should normally belong to the same scene node.
The helper compensates orbit distance when the `CameraComponent` is on a
translated child node. See [`ViewpointAnalyzer`](ViewpointAnalyzer.md) for the
draw analysis lifecycle and the [manipulation tutorial](../../tutorials/draw/11_manipulation.md).
