# M01-03 implemented image subresources and mipmaps

This records implemented contracts and the verification handoff. Compilation/runtime acceptance belongs in progress.md.

## Ranges, views and attachments

`gpu::ImageSubresourceRange` contains explicit nonzero mip and array-layer counts. An ordinary 2D image has one layer; a cubemap has six faces in `CubemapFace` order. `Image::fullRange()`, `mipExtent(level)`, `view(range)` and `fullView()` expose those resources without native handles. Odd dimensions halve with integer division and a minimum of one on each axis: 7x5 becomes 3x2, then 1x1. Explicit mip counts exceeding the complete chain fail before allocation. `minMipSize` retains its existing largest-dimension semantics.

`ImageView` borrows its image. Sampled views expose one layer as a 2D texture, or all six faces as a cubemap, over one or more mips. Storage views select exactly one color mip. Arbitrary 2D array allocation and format reinterpretation remain future extensions; the range representation can be reused. Vulkan image views and Metal texture views are cached by their owning image. Resize/reallocation/cleanup invalidates native views and descriptors referencing them; callers synchronize users and rebind resource sets.

Color/depth attachments now append a `subresource` range to the M01-02 description, preserving earlier aggregate initializers. A render target selects exactly one mip and layer. Attachment extents are compared at the selected mip. Distinct faces of one image are permitted, while duplicate selections are rejected. Both backend image convenience overloads now honor their mip parameter. The cubemap convenience overload uses the same validated attachment path. Vulkan selects a single-subresource image view; Metal selects the original texture's level and slice. Viewport/scissor use the selected extent. Existing clear/load/store, independent depth and MRT validation is retained.

`ResourceSet::setSampledImage` and `setStorageImage` accept an `ImageView` in addition to the original image pointer. The pointer overload samples the full chain, while its storage equivalent selects mip zero. `ShaderBinding` Vulkan/Metal indices and the shader authoring model remain unchanged. Bindings may be prepared before image-producing commands; callers establish the declared layouts and contents before actual shader use.

## Layouts and command sequencing

`CommandBuffer::transition(image, layout, range)` transitions selected mips/layers. Its original overload covers the whole image. Vulkan emits barriers using each selected subresource's actual tracked old layout, including dependencies for same-layout transitions. Metal uses tracked private textures and encoder boundaries, and maintains the same per-subresource API state.

`Image::currentLayout(range)` throws if the selected range contains mixed layouts. The original no-argument query requires a uniform whole image; it does not turn mixed state into Undefined. This prevents a later operation from discarding already produced mips/faces. An untouched subresource remains Undefined until explicitly transitioned. Layout bookkeeping does not prove that texels have been written: callers still supply base contents before mip generation and initialize every sampled mip/face.

The existing tracking model records intended state while commands are recorded. Callers submit work in that order and coordinate cross-queue access/ownership. Abandoning a command after recording transitions requires explicit recovery, such as synchronized recreation of affected images. Transactional command cancellation and automatic queue-family ownership transfer are outside this step.

Transfers, mip generation and transitions require recording outside active render/compute scopes. Borrowed images and views survive until GPU completion. The synchronous image conveniences require the caller to stop producers; they waitIdle, submit their staging/generation operation and wait for its completion before returning. Direct command operations let higher-level callers manage batching and resource completion.

## Allocation, upload, copy and readback

Both factories now honor ordinary 2D mip counts and exact requested image usages through native `buildImage` paths. Native Vulkan also queries the combined image-format/type/usage limits. Existing native builders delegate to that allocation path. Rebuilding an existing image waits for outstanding users. Resize preserves format/type/usages and the existing mip count, clamped to the new complete-chain length. Presentation wrappers retain their actual transfer/storage usage flags.

`Image::upload(ImageUploadDescription, finalLayout)` accepts packed texels in the destination's format, byte size, optional row pitch, mip and layer. It validates bounds/size/usage, repacks padded rows, stages the transfer and transitions only that subresource to the requested final layout (ShaderReadOnly by default). Integer and float HDR buffers use this same API. Vulkan flushes staging writes; Metal uses padded staging rows and a native blit encoder.

`uploadImage(base::Image*)` retains the CPU-image convenience through shared GPU code. It expands 1-4 source channels, treats one/two-channel images as grayscale/grayscale-alpha, handles RGBA/BGRA order, and converts normalized bytes or float data to supported normalized/half/float target formats. HDR components remain unclamped for floating-point targets. It performs no color-space transfer conversion. Integer targets require explicitly packed uploads. `uploadRGBA8` retains its RGBA8/matching-size contract. CPU-image upload initializes mip zero, layer zero; generation is explicit.

`CommandBuffer::copyImage(sourceView, destinationView)` copies one complete selected color subresource between equal formats/extents in TransferSrc/TransferDst layouts. The pointer overload copies base mip/layer zero consistently on both backends. Copies preserve layouts; callers transition afterwards. Vulkan filtered `blitImage` validates native blit/filter capabilities before scaling. Metal's exact texture-copy encoder does not rescale: its convenience blit rejects differing extents instead of silently copying an unscaled region. Fullscreen resampling remains a later pipeline operation.

`Image::readPixels(bytes, range, finalLayout)` synchronously reads one complete color mip/layer in its packed native format. It validates TransferSrc and a defined source layout, waits for completion, removes staging row padding, and finishes in the requested final layout. Vulkan invalidates host allocation caches before reading. `readPixelsRGBA8` keeps its original format and base-level behavior. Depth/stencil uploads, copies and packed readback are not added; later picking/export work may extend regions, depth formats and asynchronous readback using these same contracts.

## Mipmap generation and sampling

`FormatCapabilities::mipmapGeneration` is separate from scaled-blit support. Vulkan combines native blit source/destination and linear-filter features; Metal advertises its native mip generator for supported filterable color formats.

`CommandBuffer::generateMipmaps(image, finalLayout)` records generation for every allocated mip and face. `Image::generateMipmaps(finalLayout)` is the synchronous convenience. Vulkan uses adjacent-level filtered blits with per-range barriers. Metal uses its native mipmap generator. Multi-level generation requires initialized base-level layouts on every face, TransferSrc/TransferDst usages and supported non-integer color formats. A single-level image only receives its final transition; it needs no filtering support. Integer/depth multi-level generation and unsupported formats fail before recording. Regeneration replaces all higher mips: do not apply it after authoring independent roughness levels for specular IBL.

`SamplerDescription` appends minLod/maxLod, defaulting to zero/1000. Both backends validate and honor that interval. Vulkan no longer clamps every texture sample to level zero; a view's allocated chain bounds the available levels. Existing single-level images remain valid.

The existing cubemap example now allocates/generates a full 2D HDR source chain when the format supports generation, with a single-level fallback and capability-aware nearest/linear filtering. Its rendered specular cube mip chain remains authored by the existing per-face/per-level render pass and can be inspected through its LOD control.

Native references: [Apple texture views](https://developer.apple.com/documentation/metal/mtltexture/newtextureviewwithpixelformat%3Atexturetype%3Alevels%3Aslices%3A), [Vulkan image blits](https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdBlitImage.html) and [format features](https://docs.vulkan.org/refpages/latest/refpages/source/VkFormatFeatureFlagBits.html).

## Verification handoff

The [subresource probe](../../../../tests/gpu/subresources/README.md) extends the existing manual MRT runner. It checks padded uploads, odd dimensions, mixed layout queries, generated levels, targeted mip clears, subresource copies, every cube face/mip, native sampled-view creation, HDR values, single-level behavior and rejected ranges/formats. Example 06 supplies the textured-mesh/cubemap sampling integration and unsupported-generation fallback. Existing triangle, render-to-texture, MRT/depth and ray-query examples remain regression checks.

Agent checks cover source signatures/callers, per-mip/layer state updates, native view lifetime and staging completion, Metal API spellings against vendored headers, block/conditional balance, dependency boundaries, local links and git diff --check. No compilation, shader compilation, C++ probe execution or runtime/backend test was performed. No CMake file was changed. The lead compiles the library with the new Image.cpp source, existing draw examples and manual probe on available Vulkan/Metal backends, and reports any failures.

## M01-10 exact depth-copy extension

The initial M01-03 color-only copy limitation is extended by M01-10: exact D16_UNORM/D32_SFLOAT subresource copies now use the same layouts/usages/extent validation and Vulkan depth aspect/Metal native texture copy. Combined depth-stencil formats remain rejected. Depth upload/readback/scaled blit remain unsupported. See [raster-layers.md](raster-layers.md).
