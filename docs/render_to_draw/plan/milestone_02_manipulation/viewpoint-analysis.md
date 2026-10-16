# Draw viewpoint analysis

`bg2e::utils::ViewpointAnalyzer` is the experimental draw/GPU implementation. It is independent of the production `bg2e::render::ViewpointAnalyzer`, which remains unchanged.

Construct it with a borrowed initialized `draw::Engine`, then call `init()` or let `analyze()` initialize its resources. Configuration controls output size and uniformly sampled yaw, pitch, target height, distance scale and vertical field of view. Cyclic ranges omit their upper endpoint. `generateCandidates()` exposes the fitted camera candidates without rendering.

`analyze(scene::Node*)` extracts a complete draw queue and renders each candidate synchronously with the existing paired draw picking shaders into RGBA8 IDs and D32 depth. IDs map to `analyzedSubmeshes()` for that result set. Statistics include visible pixels, edge pixels, visible submesh count, bounds in pixel coordinates, linear depth distribution and per-submesh pixel counts. The default score rewards coverage and penalizes edge contact; override `score()` for application policy. Empty scenes return an empty sample list and clear the ID map. Nodes/submeshes outside the candidate frustum naturally receive zero pixels.

The analyzer borrows its engine and requires scene/GPU producers to be stopped during analysis. Its synchronous submissions and readbacks are intended for infrequent framing tools. D32 readback is a typed GPU image operation implemented by Vulkan and Metal; it accepts only initialized `D32_SFLOAT` images created with `TransferSrc` usage. Unsupported formats fail explicitly. No ray tracing is used.

`utils::applyViewpointToOrbitCamera()` has an overload for the new candidate type. It updates only orbit target, yaw/pitch and distance. It preserves the camera's projection matrix and all projection parameters, including perspective FOV, optical focal length and clipping planes. Candidate FOV/near/far values belong to the analyzer's internal rendering and are not applied to the scene camera. The existing overload for `render::CameraCandidate` remains available and unchanged.
