# Image subresource and mipmap probe

`probe.hpp` provides `bg2e::gpu::tests::runSubresourceProbe(Device&)` for an initialized GPU device outside an active frame/recording operation. It uses generic GPU primitives, without a draw engine or scene graph.

The existing [manual MRT runner](../mrt_probe/README.md) now calls it after the MRT/depth checks. Recompile that runner and use its same Vulkan/Metal arguments and paired shader files. This probe needs no additional shaders or CMake changes. It can also be called from a local GPU harness independently of the MRT helper.

Expected checks:

- A padded-row 7x5 RGBA upload allocates 7x5, 3x2 and 1x1 levels. Only mip zero initially changes layout; a whole-image query rejects mixed layouts.
- Generated mip levels retain a constant input color and finish ShaderReadOnly.
- Rendering a clear through `beginRendering(image, 1)` changes mip one, preserving mips zero and two.
- Copying mip one into a separate 3x2 image preserves its texels and dimensions.
- Six distinct cube faces survive native generation at every allocated mip.
- Sampled resource-set bindings create a selected 2D mip view and a selected cubemap mip view. Actual shader sampling is exercised separately by example 06.
- Float HDR values above one and below zero survive packed upload/readback. A one-level image accepts generation as a final transition.
- Invalid ranges, undersized uploads, impossible mip counts and filtered integer mip generation fail explicitly.

Run the existing MRT/depth checks and this probe on available Vulkan and Metal devices with validation enabled. Example `06_cubemap_render_pass` additionally checks texture sampling: vary the specular LOD and inspect the generated/filtered result. Its 2D HDR source uses a complete generated chain when supported and falls back to one level otherwise. Existing triangle, render-to-texture and query-shadow examples are regression checks.

The agent did not compile or execute these fixtures. Runtime results and device coverage remain for the project lead to report.
