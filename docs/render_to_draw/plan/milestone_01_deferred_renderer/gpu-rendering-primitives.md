# M01-02 implemented GPU rendering primitives

This records actual interfaces and the handoff, not runtime acceptance or a milestone review.

## API

`gpu::RenderingDescription` contains an ordered `colors` array and optional `depth`. Each attachment supplies its image, load/store operation and clear values. Colors use `ColorClearValue` with floating-point, unsigned-integer and signed-integer representations selected by the attachment format. Depth includes depth/stencil clears; combined depth/stencil formats use the same load/store policy for both aspects. Zero colors with depth is supported. Images are borrowed through GPU completion.

`CommandBuffer::beginRendering(const RenderingDescription&)` records this description on Vulkan/Metal. Native render scopes remain lazily materialized, allowing `clearColor(index, value)` and `clearDepth(value)` before the first bind/draw. Clears after materialization fail explicitly; integer clears use the description's typed fields. Clear-only passes materialize at endRendering. Existing surface, one-color, color/depth and cubemap overloads remain. Existing Metal depth targets now store their contents rather than discard them; use the explicit description for deliberate load/store control.

The explicit path currently accepts non-aliasing single-level 2D attachments with matching extents. Transition colors to `ColorAttachment` and depth to `DepthAttachment` before opening a pass. The caller orders dependent passes with transitions, including same-layout dependencies on Vulkan. M01-03 extends attachment targeting and state tracking to mip/layer views; existing cubemap paths remain independent until that work.

`GraphicsPipelineDescription::colorAttachments` specifies ordered format/blend/write-mask state. It replaces legacy `colorFormat` when nonempty; setting both fails. Optional `depthState` independently specifies test, write and comparison. Unset state preserves the historical per-backend comparison defaults (Vulkan LessOrEqual, Metal Less); explicit DepthState gives matching behavior. Disabling tests with writes enabled means unconditional depth writes on both backends. No MSAA configuration was added.

`GraphicsPipeline::validateDescription`, `CommandBuffer::validateRendering` and `validatePipelineAttachments` expose preflight checks. Constructors and the explicit recording/binding path call them before native creation/recording: count, format category/support, extent, image type/usage, device ownership, layout, duplicate targets, shader stage/validity, independent blend support and aggregate color storage limits are checked. Format/shader resource bindings still require the native shader compiler and pipeline creation to succeed.

`Image::usage()` reports allocated primitive usages. Native Image builders now synchronize their inherited DeviceResource device identity, which was previously absent for default-constructed surface targets. Metal 2D image creation honors combined sampled/storage/color usage and tracked hazards instead of dropping attachment usage.

## Capabilities and formats

`PhysicalDeviceProperties::rendering` and `Device::renderingCapabilities()` expose attachment/dimension limits and independent blend. PhysicalDevice and Device expose `formatCapabilities(format)` with supported usages, blending/filtering and attachment storage cost. Vulkan queries native optimal-tiling format features and device limits. Metal follows documented family limits and format categories, with native selectors for depth24 and float32 filtering. Its aggregate color cost uses a conservative eight-byte floor on Apple GPUs for small formats that can expand in tile memory; valid configurations close to the limit can be rejected conservatively. Native allocation/pipeline creation remains the final validator. Dimension limits are also conservative for newer Metal families.

Added formats: R8/RG8 normalized; R16/RG16 and R32/RG32 float; R32 unsigned/signed integer, RG32 unsigned integer and RGBA32 unsigned integer. Existing format enum values are preserved. Both backend mapping directions cover every exposed PixelFormat. Blending/filtering is unavailable for integer formats.

RayTracingCapabilities now distinguishes `querySupported()` and `pipelineSupported()`; legacy `fullSupported()` remains. Device reports enabled capabilities separately from physical support. `rayQueryInRenderStages` additionally distinguishes Metal compute intersection from the separately queried ability to intersect in raster stages. Vulkan enables query and pipeline extensions independently and loads acceleration functions for query-only devices. Metal reports its existing compute/intersection tracing execution model; this is not a native SBT/stage guarantee or an implementation of the deferred M03 design.

Native reference sources: [Apple Metal feature tables](https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf), [Vulkan dynamic rendering](https://docs.vulkan.org/features/latest/features/proposals/VK_KHR_dynamic_rendering.html) and [Vulkan physical device features](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceFeatures.html).

## Verification handoff

The manual [MRT probe](../../../../tests/gpu/mrt_probe/README.md) includes a standalone runner, GPU-only helper and paired GLSL/Metal shader fixtures. It checks six distinct colors, depth store/load, independent depth-write disable and rejected configurations. It is not registered as a build target because CMake edits are not authorized. Compile/link it with the existing GPU-example settings, or invoke its helper from a local GPU harness.

Agent checks: source diff/ownership/signatures, existing triangle/cubemap callers, all PixelFormat mappings, Metal enum/method spellings against vendored headers, block balance, absence of high-level GPU dependencies and `git diff --check`. No compilation, shader compilation or runtime execution was performed. The lead should compile the library/existing GPU examples on available backends, run the probe and check blending/write masks, integer clears, depth-only and unsupported cases. Backend coverage remains unspecified until reported.
