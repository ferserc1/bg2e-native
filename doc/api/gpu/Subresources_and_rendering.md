# Rendering attachments, subresources and transfer

Headers: `<bg2e/gpu/Common.hpp>`, `Image.hpp`, `CommandBuffer.hpp`,
`GraphicsPipeline.hpp`, `ResourceSet.hpp`, `PhysicalDevice.hpp`.

The multi-attachment rendering API accepts ordered color attachments with
independent load/store and clear values plus optional depth. Pipelines specify
matching formats, depth state, raster state and per-target blend/write masks.
Validation checks device limits and attachment extents/formats before recording.
These are single-sample paths; MRT support is not an MSAA promise. Query primitive,
format, filtering, storage and mip-generation capabilities independently.

`ImageSubresourceRange` explicitly selects mip/layer counts. Image2D has one
layer; cubemaps have six faces. `mipExtent`, `view`, `fullView` and `fullRange`
provide native-free selection. Render attachments select exactly one mip/face;
resource sets accept sampled/storage ImageViews. Sampled views select one layer
or all six cubemap faces; storage selects one color mip. Arbitrary 2D arrays and
format reinterpretation are not provided. Views borrow the owning image and
become invalid on rebuild/cleanup; descriptors must be rebound after synchronization.

`CommandBuffer::transition` tracks per-subresource layouts. Mixed-range
`currentLayout` queries throw; Undefined must not conceal valid contents in other
mips. Layout state does not prove texels were initialized. Transfers/transitions/
mip generation occur outside active rendering/compute scopes.

`Image::upload(description, finalLayout)` accepts destination-format packed
texels, byte size, row pitch, mip/layer. `uploadImage(base::Image*)` expands CPU
channels and converts supported normalized/float representations without a color
transfer conversion. Integer targets need packed uploads. `generateMipmaps`
requires initialized base contents for each face and supported filterable color
formats with transfer usages; authored roughness mips must not be regenerated.
Odd mip dimensions halve with floor and a minimum of one.

Exact `copyImage` selects equal-format/equal-extent subresources. D16_UNORM and
D32_SFLOAT exact depth copies are supported; packed depth-stencil, depth upload
and depth readback are not. Vulkan filtered blit checks native capabilities;
Metal exact-copy convenience rejects scaling. `readPixels(bytes, range, finalLayout)`
synchronously returns one tight native-format color mip/layer, removes row
padding and restores the requested layout. It is not asynchronous selection
readback or image-format conversion.

Synchronous conveniences stop producers, drain outstanding work and finish
staging before return. Direct command operations let the caller batch work;
borrowed resources must survive submission completion. ResourceBinding declares the Vulkan set and ShaderBinding
contains explicit Vulkan/Metal indices for buffer/texture/sampler resources: those
namespaces are not interchangeable or inferred from each other. See
[shader libraries](ShaderLibraries.md) and [submission tracking](Submission_tracking_and_waitIdle.md).

The concrete entry point is `CommandBuffer::beginRendering(RenderingDescription)`.
`colors` contains ColorAttachmentDescription entries; optional `depth` supplies a
DepthAttachmentDescription. Each selects an image/subresource and load/store/clear
policy. `Device::renderingCapabilities()` and `PhysicalDevice::formatCapabilities(format)`
report limits/usages separately from ray tracing capabilities. Consult the public
headers for overload defaults rather than assume support from a pixel format enum.
