# Paired shader library probe

Manual M01-04 fixture; no new executable registration. Compile the existing [MRT runner](../mrt_probe/README.md) as described there. The optional third argument selects the **engine shaders root**, containing `draw/gbuffer` and `draw/composite`:

```text
mrt_probe vulkan MRT_SHADER_DIRECTORY ENGINE_SHADER_DIRECTORY
mrt_probe metal MRT_SHADER_DIRECTORY ENGINE_SHADER_DIRECTORY
```

The first directory still contains the separate MRT fixture shaders. The engine shader tree is produced by the bg2e build. It can come from development output, a direct install, packaged app resources or a standalone project. The runner also performs the previous MRT/subresource checks. Without the optional argument, its prior behavior is preserved.

`runShaderLibraryProbe` creates modules and native graphics pipelines using identically named stages in two pipeline folders. Each draws a fullscreen triangle into an odd-sized 7x5 linear RGBA8 target. Readback must distinguish the normal-incidence BRDF (approximately 0.204) from composite color correction (approximately 0.654). This tests entry points and prevents a flattened deployment from appearing correct merely because modules open. It also creates a compute pipeline with explicit Vulkan set 0/binding 0 and Metal texture 0, dispatches the composite image-fill kernel and checks the same corrected color, including alpha and edge bounds.

Run on available Vulkan/Metal backends with validation enabled; the target requires RGBA8 color/storage/readback support. Confirm a shared include edit rebuilds dependent native stages and app resources. Repeat against deployed directories and check existing production shader users. No agent build or execution was performed.
