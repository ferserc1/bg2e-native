# Draw viewpoint analyzer checks

This is a manual project-lead checklist. The analyzer uses synchronous draw/GPU raster work and has no separate executable in this step.

- Build the library with the new utility and GPU image API on available Vulkan and Metal configurations. Confirm the paired `draw/picking/picking` shaders are present in the deployed shader library.
- On a loaded draw scene with asymmetric geometry and several submeshes, inspect candidate count/order, projected bounds, pixels per submesh, visible-submesh count, border counts and finite linear depth statistics. Confirm the best sample favors broad coverage without edge clipping.
- Include a scene with hidden submeshes, a disabled subtree and a component whose resource is not loaded. Confirm queue extraction excludes them consistently with ordinary draw rendering.
- Check an empty or fully hidden scene returns no samples and an empty submesh map. Check candidates with no visible geometry have zeroed statistics and finite scores.
- Exercise a model with multiple visible submesh identities and verify IDs map back to the corresponding node, DrawableComponent and submesh index.
- Check cyclic yaw excludes the maximum endpoint; non-cyclic ranges include both endpoints. Invalid ranges, zero extents and invalid bounds should fail clearly without corrupting the previous configuration. Keep otherwise valid candidate batches to a practical size; work and result memory scale with their count.
- Confirm D32 typed readback returns one float per pixel on Vulkan and Metal, preserves the requested final layout, and rejects non-D32 images, uninitialized images and images without `TransferSrc` usage.
- Apply a candidate to orbit cameras with perspective, optical and unsupported projection types. Verify target/rotation/distance updates while the projection object, matrix, FOV/focal length and near/far remain unchanged, including after the normal scene update. Use different candidate and camera projection values so accidental copies are detectable; confirm the production `render::ViewpointAnalyzer` and camera overload still behave as before.
- Run with ray tracing unavailable/disabled. The analyzer must remain raster-only and independent of RT capability.

The agent performed static inspection only; no build, shader compilation or runtime checks were run.
