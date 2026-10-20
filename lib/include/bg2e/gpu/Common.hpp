/*
 *    business grade graphic engine (bg2e engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <algorithm>

#include <bg2e/common.hpp>

#include <cstdint>
#include <cstddef>
#include <array>
#include <optional>
#include <string>
#include <vector>
#include <stdexcept>

namespace bg2e {
namespace gpu {

enum class BackendType
{
    Vulkan,
    Metal
};

struct Size2D {
    uint32_t width = 0;
    uint32_t height = 0;

    Size2D() = default;
    Size2D(uint32_t w, uint32_t h) : width(w), height(h)
    {
    }

    bool operator==(const Size2D& o) const
    {
        return width == o.width && height == o.height;
    }
    bool operator!=(const Size2D& o) const
    {
        return !(*this == o);
    }
    bool isZero() const
    {
        return width == 0 || height == 0;
    }
};

struct Size3D {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;

    Size3D() = default;
    Size3D(uint32_t w, uint32_t h, uint32_t d = 1) : width(w), height(h), depth(d)
    {
    }
    explicit Size3D(const Size2D& s, uint32_t d = 1) : width(s.width), height(s.height), depth(d)
    {
    }

    Size2D toSize2D() const
    {
        return Size2D{width, height};
    }

    bool operator==(const Size3D& o) const
    {
        return width == o.width && height == o.height && depth == o.depth;
    }
    bool operator!=(const Size3D& o) const
    {
        return !(*this == o);
    }
};

enum class PixelFormat
{
    Undefined = 0,

    // --- Color ---
    R8G8B8A8_UNORM,
    R8G8B8A8_SRGB,
    B8G8R8A8_UNORM,
    B8G8R8A8_SRGB,
    R16G16B16A16_SFLOAT,
    R32G32B32A32_SFLOAT,

    // --- Depth / stencil ---
    D16_UNORM,
    D32_SFLOAT,
    D24_UNORM_S8_UINT,
    D32_SFLOAT_S8_UINT,

    // Additional scalar/vector and integer render/storage formats.
    R8_UNORM,
    R8G8_UNORM,
    R16_SFLOAT,
    R16G16_SFLOAT,
    R32_SFLOAT,
    R32G32_SFLOAT,
    R32_UINT,
    R32_SINT,
    R32G32_UINT,
    R32G32B32A32_UINT,
};

constexpr bool isDepthFormat(PixelFormat f)
{
    return f == PixelFormat::D16_UNORM || f == PixelFormat::D32_SFLOAT || f == PixelFormat::D24_UNORM_S8_UINT || f == PixelFormat::D32_SFLOAT_S8_UINT;
}

constexpr bool hasStencil(PixelFormat f)
{
    return f == PixelFormat::D24_UNORM_S8_UINT || f == PixelFormat::D32_SFLOAT_S8_UINT;
}

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    Color() = default;
    Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_)
    {
    }
};

enum class ImageLayout
{
    Undefined = 0,
    General,
    ColorAttachment,
    DepthAttachment,
    ShaderReadOnly,
    TransferSrc,
    TransferDst,
    Present
};

enum class ShaderStage
{
    Vertex,
    Fragment,
    Compute,
    RayGeneration,
    Miss,
    ClosestHit,
    AnyHit
};

enum class PipelineBarrierFlags : uint32_t
{
    None = 0,
    AllCommands = 1 << 0,
    VertexInput = 1 << 1,
    VertexShader = 1 << 2,
    FragmentShader = 1 << 3,
    ComputeShader = 1 << 4,
    Transfer = 1 << 5,
    ColorAttachmentOutput = 1 << 6,
    EarlyFragment = 1 << 7,
    LateFragment = 1 << 8,
    BottomOfPipe = 1 << 9,
    TopOfPipe = 1 << 10,
    MemoryRead = 1 << 11,
    MemoryWrite = 1 << 12
};

inline PipelineBarrierFlags operator|(PipelineBarrierFlags a, PipelineBarrierFlags b)
{
    return static_cast<PipelineBarrierFlags>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline PipelineBarrierFlags operator&(PipelineBarrierFlags a, PipelineBarrierFlags b)
{
    return static_cast<PipelineBarrierFlags>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline PipelineBarrierFlags& operator|=(PipelineBarrierFlags& a, PipelineBarrierFlags b)
{
    return a = a | b;
}

inline bool hasFlag(PipelineBarrierFlags flags, PipelineBarrierFlags flag)
{
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

struct ShaderConstant {
    uint32_t id = 0;
    uint32_t value = 0;
};

struct ShaderModuleDescription {
    std::string filePath;            // Vulkan: .spv ; Metal: .metallib
    std::string entryPoint = "main"; // Vulkan: SPIR-V entry ; Metal: MTL function name
    ShaderStage stage = ShaderStage::Vertex;
    std::string debugName; // Debug label for validation layers / Xcode GPU Capture
    std::vector<ShaderConstant> constants; // UInt32 Vulkan specialization / Metal function constants.
};

// Push constant range descriptor.
//
// The `offset` field is the Vulkan push constant offset. Metal ignores it for
// binding purposes — Metal push constant buffer indices are fixed by bg2e::gpu:
//
//   Vertex   -> [[buffer(1)]]
//   Fragment -> [[buffer(0)]]
//   Compute  -> [[buffer(0)]]
//
// In Metal vertex shaders, [[buffer(0)]] is reserved for the geometric vertex
// buffer because bg2e::gpu always binds vertex data as a single vertex buffer.
//
// Only one PushConstantRange per ShaderStage is allowed. Duplicates are
// rejected at PipelineLayout creation time.
struct PushConstantRange {
    uint32_t offset = 0;                     // Vulkan push constant offset (ignored by Metal)
    uint32_t size = 0;                       // Vulkan minimum guaranteed: 128 bytes; Metal limit: ~4 KB
    ShaderStage stage = ShaderStage::Vertex; // single shader stage (not a bitmask)
};

// --- Resource binding types ---------------------------------------------------

enum class ResourceType
{
    UniformBuffer,        // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER / Metal [[buffer(N)]]
    StorageBuffer,        // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER / Metal [[buffer(N)]]
    SampledImage,         // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE  / Metal [[texture(N)]]
    StorageImage,         // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE  / Metal [[texture(N)]] (read-write)
    Sampler,              // VK_DESCRIPTOR_TYPE_SAMPLER        / Metal [[sampler(N)]]
    AccelerationStructure // VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR / Metal [[buffer(N)]]
                          // (instance_acceleration_structure argument)
};

// Backend-specific binding indices.
//
// vulkan — Vulkan descriptor binding index (within a descriptor set).
// metal  — Metal argument index: [[buffer(N)]], [[texture(N)]] or
//          [[sampler(N)]] depending on the ResourceType.
//
// Vulkan descriptor set/binding indices and Metal resource indices are
// different concepts. There is no 1:1 mapping between them.
//
// Push constants do NOT use ShaderBinding. The existing numeric value in
// PushConstantRange::offset is the Vulkan push constant offset. Metal push
// constant buffer indices are fixed by bg2e::gpu convention (see
// PushConstantRange docs).
struct ShaderBinding {
    uint32_t vulkan = 0;
    uint32_t metal = 0;
};

// Resource binding descriptor for a single shader resource.
//
// For Metal buffer resources (UniformBuffer, StorageBuffer), the metal index
// in `binding` must respect the reserved-buffer rules enforced by the
// PipelineLayout validator:
//
//   Vertex stage:   metal index >= 2  (buffer(0)=geometry, buffer(1)=push constants)
//   Fragment stage: metal index >= 1  (buffer(0)=push constants)
//   Compute stage:  metal index >= 1  (buffer(0)=push constants)
//
// Texture and sampler bindings (SampledImage, StorageImage, Sampler) are not
// affected because Metal [[texture(N)]] and [[sampler(N)]] use separate
// namespaces from [[buffer(N)]].
struct ResourceBinding {
    uint32_t set = 0;
    ShaderBinding binding = {};
    ResourceType type = ResourceType::SampledImage;
    ShaderStage stage = ShaderStage::Fragment;
    uint32_t count = 1; // Fixed descriptor count, fully populated before use.
    std::vector<ShaderStage> additionalStages; // Native Vulkan visibility union.
    // For sampled-image/sampler arrays: explicit Metal buffer slot; binding.metal
    // is the first [[id(N)]] inside that argument buffer. UINT32_MAX = direct.
    uint32_t metalArgumentBuffer = UINT32_MAX;
    // Metal argument array. When metalArgumentRecordsIndexable is true, encode
    // independently stride-indexed single-resource records (Tier-2 pointer path).
    // Otherwise encode one ordinary Metal argument buffer with fixed arrays.
    bool metalArgumentRecordArray = false;
    bool metalArgumentRecordsIndexable = false;
};

// Pipeline layout description.
//
// Defines push constant ranges and resource bindings for a pipeline.
// Validation is performed at PipelineLayout creation time:
//
//   - At most one PushConstantRange per ShaderStage (both Vulkan and Metal).
//   - For Metal, UniformBuffer/StorageBuffer bindings must use buffer indices
//     that do not collide with reserved slots (see ResourceBinding docs).
//
// Example — vertex push constants + vertex UBO:
//
//   gpu::PipelineLayoutDescription layoutDesc{};
//   layoutDesc.pushConstants.push_back(
//       {0, sizeof(CameraPushConstants), gpu::ShaderStage::Vertex}
//   );
//   layoutDesc.resourceBindings.push_back({
//       0,
//       {.vulkan = 0, .metal = 2},
//       gpu::ResourceType::UniformBuffer,
//       gpu::ShaderStage::Vertex,
//       1
//   });
//
// Corresponding Vulkan (GLSL):
//   layout(push_constant) uniform CameraPushConstants { mat4 proj; mat4 view; } cam;
//   layout(set = 0, binding = 0) uniform ModelUBO { mat4 model; } model;
//
// Corresponding Metal:
//   constant CameraPushConstants& cam   [[buffer(1)]];
//   constant ModelUBO&            model [[buffer(2)]];
struct PipelineLayoutDescription {
    std::vector<PushConstantRange> pushConstants;
    std::vector<ResourceBinding> resourceBindings;
    std::string debugName; // Debug label for validation layers / Xcode GPU Capture
};

// Validate that at most one PushConstantRange is declared per ShaderStage.
// Throws std::runtime_error on failure.
inline void validatePushConstantRanges(const std::vector<PushConstantRange>& ranges)
{
    uint32_t seen = 0;
    for (const auto& range : ranges)
    {
        const auto stage = static_cast<uint32_t>(range.stage);
        if (stage > static_cast<uint32_t>(ShaderStage::AnyHit) ||
            (seen & (1u << stage)) || range.size == 0 || range.size % 4 || range.offset % 4)
        {
            throw std::runtime_error("Invalid or duplicate push constant stage/range");
        }
        seen |= 1u << stage;
    }
}

// Validate Metal buffer indices for UniformBuffer/StorageBuffer bindings.
//
// In Metal, push constants and the vertex buffer consume buffer argument slots:
//   Vertex stage:   buffer(0) = geometry, buffer(1) = push constants -> UBO/SSBO >= 2
//   Fragment stage: buffer(0) = push constants -> UBO/SSBO >= 1
//   Compute stage:  buffer(0) = push constants -> UBO/SSBO >= 1
//
// This restriction does not apply to textures or samplers because Metal
// [[texture(N)]] and [[sampler(N)]] are separate namespaces.
//
// Throws std::runtime_error on failure.
inline void validateMetalBufferBindings(const std::vector<ResourceBinding>& bindings)
{
    for (const auto& rb : bindings)
    {
        bool isBufferType = (rb.type == ResourceType::UniformBuffer ||
                             rb.type == ResourceType::StorageBuffer ||
                             rb.type == ResourceType::AccelerationStructure ||
                             rb.metalArgumentBuffer != UINT32_MAX);
        if (!isBufferType)
        {
            continue;
        }

        const auto metalIndex = rb.metalArgumentBuffer == UINT32_MAX ? rb.binding.metal : rb.metalArgumentBuffer;
        switch (rb.stage)
        {
        case ShaderStage::Vertex:
            if (metalIndex < 2)
            {
                throw std::runtime_error(
                    "PipelineLayout validation failed: Metal vertex buffer resource binding "
                    "uses index " +
                    std::to_string(rb.binding.metal) + ", but vertex-stage "
                                                       "UniformBuffer/StorageBuffer bindings must use index >= 2. buffer(0) is "
                                                       "reserved for the geometric vertex buffer and buffer(1) is reserved for "
                                                       "vertex push constants.");
            }
            break;
        case ShaderStage::Fragment:
            if (metalIndex < 1)
            {
                throw std::runtime_error(
                    "PipelineLayout validation failed: Metal fragment buffer resource binding "
                    "uses index " +
                    std::to_string(rb.binding.metal) + ", but fragment-stage "
                                                       "UniformBuffer/StorageBuffer bindings must use index >= 1. buffer(0) is "
                                                       "reserved for fragment push constants.");
            }
            break;
        case ShaderStage::Compute:
            if (metalIndex < 1)
            {
                throw std::runtime_error(
                    "PipelineLayout validation failed: Metal compute buffer resource binding "
                    "uses index " +
                    std::to_string(rb.binding.metal) + ", but compute-stage "
                                                       "UniformBuffer/StorageBuffer bindings must use index >= 1. buffer(0) is "
                                                       "reserved for compute push constants.");
            }
            break;
        case ShaderStage::RayGeneration:
        case ShaderStage::Miss:
        case ShaderStage::ClosestHit:
        case ShaderStage::AnyHit:
            if (metalIndex < 1)
            {
                throw std::runtime_error(
                    "PipelineLayout validation failed: Metal ray tracing buffer resource binding "
                    "uses index " +
                    std::to_string(rb.binding.metal) + ", but ray-tracing-stage "
                                                       "UniformBuffer/StorageBuffer bindings must use index >= 1. buffer(0) is "
                                                       "reserved for push constants.");
            }
            break;
        }
    }
}

inline constexpr uint32_t MaxSampledResourceTableCapacity = 16384;

// Enabled logical-device limits for fully populated sampled-resource arrays.
// Limits include ordinary resources; callers reserve their other bindings first.
struct ResourceArrayCapabilities {
    bool nonUniformSampledResources = false;
    uint32_t maxSampledImages = 0;
    uint32_t maxSamplers = 0;
    uint32_t maxResourcesPerStage = 0;
    uint64_t maxStorageBufferRange = 0;

    uint32_t profileCapacity(uint32_t reservedImages = 0, uint32_t reservedSamplers = 0,
                             uint32_t reservedResources = 0) const
    {
        if (!nonUniformSampledResources || maxSampledImages <= reservedImages ||
            maxSamplers <= reservedSamplers || maxResourcesPerStage <= reservedResources)
        {
            return 0;
        }
        // Exact device-clamped capacity, not rounded down to a compiled profile.
        return std::min({MaxSampledResourceTableCapacity, maxSampledImages - reservedImages,
            maxSamplers - reservedSamplers, (maxResourcesPerStage - reservedResources) / 2});
    }
};

// Arrays are sampled images/samplers only. Other resources remain scalar.
inline void validateResourceArrays(const PipelineLayoutDescription& description,
                                   const ResourceArrayCapabilities& caps)
{
    std::array<uint64_t, 7> images{}, samplers{}, resources{};
    for (size_t i = 0; i < description.resourceBindings.size(); ++i)
    {
        const auto& rb = description.resourceBindings[i];
        if (rb.count == 0 || (rb.count > 1 && rb.type != ResourceType::SampledImage && rb.type != ResourceType::Sampler))
        {
            throw std::invalid_argument("Only nonempty sampled-image/sampler arrays are supported");
        }
        if ((rb.count > 1 || rb.metalArgumentBuffer != UINT32_MAX) && !caps.nonUniformSampledResources)
        {
            throw std::runtime_error("Nonuniform sampled-resource arrays are unavailable");
        }
        for (size_t j = 0; j < i; ++j)
        {
            const auto& other = description.resourceBindings[j];
            if (rb.set == other.set && rb.binding.vulkan == other.binding.vulkan)
            {
                throw std::invalid_argument("Duplicate Vulkan set/binding; use additionalStages for visibility");
            }
        }
        auto addStage = [&](ShaderStage stage)
        {
            const auto index = static_cast<size_t>(stage);
            if (index >= resources.size())
            {
                throw std::invalid_argument("Invalid resource shader stage");
            }
            resources[index] += rb.count;
            images[index] += rb.type == ResourceType::SampledImage ? rb.count : 0;
            samplers[index] += rb.type == ResourceType::Sampler ? rb.count : 0;
        };
        addStage(rb.stage);
        for (auto stage : rb.additionalStages)
        {
            if (stage == rb.stage || std::count(rb.additionalStages.begin(), rb.additionalStages.end(), stage) != 1)
            {
                throw std::invalid_argument("Duplicate resource stage visibility");
            }
            addStage(stage);
        }
    }
    // Enforce limits only when using the new array path, preserving scalar callers.
    const bool arrays = std::any_of(description.resourceBindings.begin(), description.resourceBindings.end(),
        [](const auto& rb)
        {
            return rb.count > 1 || rb.metalArgumentBuffer != UINT32_MAX;
        });
    if (arrays)
    {
        uint64_t totalImages = 0, totalSamplers = 0;
        for (const auto& rb : description.resourceBindings)
        {
            totalImages += rb.type == ResourceType::SampledImage ? rb.count : 0;
            totalSamplers += rb.type == ResourceType::Sampler ? rb.count : 0;
        }
        if (totalImages > caps.maxSampledImages || totalSamplers > caps.maxSamplers)
        {
            throw std::invalid_argument("Pipeline sampled-resource table exceeds device limits");
        }
        for (size_t i = 0; i < resources.size(); ++i)
        {
            if (images[i] > caps.maxSampledImages || samplers[i] > caps.maxSamplers || resources[i] > caps.maxResourcesPerStage)
            {
                throw std::invalid_argument("Pipeline resources exceed per-stage limits");
            }
        }
    }
}

// --- Sampler description vocabulary ------------------------------------------

enum class Filter
{
    Nearest,
    Linear
};
enum class MipmapFilter
{
    Nearest,
    Linear
};
enum class AddressMode
{
    Repeat,
    ClampToEdge,
    MirroredRepeat,
    ClampToBorder
};

struct SamplerDescription {
    Filter minFilter = Filter::Linear;
    Filter magFilter = Filter::Linear;
    MipmapFilter mipFilter = MipmapFilter::Nearest;
    AddressMode addressModeU = AddressMode::Repeat;
    AddressMode addressModeV = AddressMode::Repeat;
    AddressMode addressModeW = AddressMode::Repeat;
    std::string debugName; // Debug label for validation layers / Xcode GPU Capture
    float minLod = 0.0f;
    float maxLod = 1000.0f;
    float mipLodBias = 0.0f;
};

// --- Image usage flags -------------------------------------------------------

enum class ImageUsage : uint32_t
{
    None = 0,
    ColorAttachment = 1 << 0,
    DepthStencil = 1 << 1,
    Sampled = 1 << 2,
    Storage = 1 << 3,
    TransferSrc = 1 << 4,
    TransferDst = 1 << 5,
    Present = 1 << 6
};

inline ImageUsage operator|(ImageUsage a, ImageUsage b)
{
    return static_cast<ImageUsage>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ImageUsage operator&(ImageUsage a, ImageUsage b)
{
    return static_cast<ImageUsage>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline ImageUsage& operator|=(ImageUsage& a, ImageUsage b)
{
    return a = a | b;
}

inline bool hasFlag(ImageUsage flags, ImageUsage flag)
{
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}


// Single-sample rendering state vocabulary.
enum class CompareOperation
{
    Never, Less, Equal, LessOrEqual, Greater, NotEqual, GreaterOrEqual, Always
};
enum class BlendFactor
{
    Zero, One, SourceColor, OneMinusSourceColor, DestinationColor,
    OneMinusDestinationColor, SourceAlpha, OneMinusSourceAlpha,
    DestinationAlpha, OneMinusDestinationAlpha
};
enum class BlendOperation
{
    Add, Subtract, ReverseSubtract, Min, Max
};
enum class ColorWriteMask : uint32_t
{
    None = 0, Red = 1, Green = 2, Blue = 4, Alpha = 8, All = 15
};
inline ColorWriteMask operator|(ColorWriteMask a, ColorWriteMask b)
{
    return static_cast<ColorWriteMask>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
struct BlendState {
    bool enabled = false;
    BlendFactor sourceColor = BlendFactor::One;
    BlendFactor destinationColor = BlendFactor::Zero;
    BlendOperation colorOperation = BlendOperation::Add;
    BlendFactor sourceAlpha = BlendFactor::One;
    BlendFactor destinationAlpha = BlendFactor::Zero;
    BlendOperation alphaOperation = BlendOperation::Add;
    ColorWriteMask writeMask = ColorWriteMask::All;
    bool operator==(const BlendState&) const = default;
};
struct DepthState {
    bool testEnabled = true;
    bool writeEnabled = true;
    CompareOperation compare = CompareOperation::LessOrEqual;
};
struct ColorAttachmentState {
    PixelFormat format = PixelFormat::Undefined;
    BlendState blend;
};
enum class AttachmentLoadOperation
{
    Load, Clear, DontCare
};
enum class AttachmentStoreOperation
{
    Store, DontCare
};
// Counts are explicit, nonzero and never mean "remaining". Array layers are
// cube faces in CubemapFace order, or layer zero for an ordinary 2D image.
struct ImageSubresourceRange {
    uint32_t baseMipLevel = 0;
    uint32_t mipCount = 1;
    uint32_t baseArrayLayer = 0;
    uint32_t layerCount = 1;
    bool operator==(const ImageSubresourceRange&) const = default;
};
class Image;
// Borrowed view: its image and cached native view must survive GPU completion.
// One layer denotes a 2D view; all six faces denote a cube sampling view.
struct ImageView {
    Image* image = nullptr;
    ImageSubresourceRange range;
};
struct ImageUploadDescription {
    const void* data = nullptr; // Packed target-format texels; no implicit conversion.
    size_t byteSize = 0;
    size_t bytesPerRow = 0; // Zero selects tight packing.
    uint32_t mipLevel = 0;
    uint32_t arrayLayer = 0;
};
struct ColorClearValue {
    Color floatingPoint;
    std::array<uint32_t, 4> unsignedInteger = {};
    std::array<int32_t, 4> signedInteger = {};
};
struct ColorAttachmentDescription {
    Image* image = nullptr; // Borrowed until GPU completion.
    AttachmentLoadOperation load = AttachmentLoadOperation::Load;
    AttachmentStoreOperation store = AttachmentStoreOperation::Store;
    ColorClearValue clear;
    ImageSubresourceRange subresource; // Exactly one mip and one layer.
};
struct DepthAttachmentDescription {
    Image* image = nullptr;
    AttachmentLoadOperation load = AttachmentLoadOperation::Load;
    AttachmentStoreOperation store = AttachmentStoreOperation::Store;
    float clearDepth = 1.0f;
    uint32_t clearStencil = 0;
    ImageSubresourceRange subresource;
};
struct RenderingDescription {
    std::vector<ColorAttachmentDescription> colors;
    std::optional<DepthAttachmentDescription> depth;
};
struct RenderingCapabilities {
    uint32_t maxColorAttachments = 0;
    uint32_t maxImageDimension2D = 0;
    // Zero means no additional aggregate color storage limit.
    uint32_t maxColorAttachmentBytesPerSample = 0;
    bool independentBlend = false;
};
struct FormatCapabilities {
    ImageUsage usages = ImageUsage::None;
    bool blend = false;
    bool linearFiltering = false;
    bool blitSource = false; // Scaled blit support; exact copies use transfer usages.
    bool blitDestination = false;
    bool mipmapGeneration = false;
    uint32_t colorAttachmentBytesPerSample = 0;
    bool supports(ImageUsage requested) const
    {
        return (static_cast<uint32_t>(usages) & static_cast<uint32_t>(requested)) == static_cast<uint32_t>(requested);
    }
};
inline bool isUnsignedIntegerFormat(PixelFormat format)
{
    return format == PixelFormat::R32_UINT || format == PixelFormat::R32G32_UINT ||
           format == PixelFormat::R32G32B32A32_UINT;
}
inline bool isSignedIntegerFormat(PixelFormat format)
{
    return format == PixelFormat::R32_SINT;
}
inline uint32_t pixelFormatBytes(PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R8_UNORM:
        return 1;
    case PixelFormat::R8G8_UNORM:
        return 2;
    case PixelFormat::R16_SFLOAT:
        return 2;
    case PixelFormat::R16G16_SFLOAT:
        return 4;
    case PixelFormat::R32_SFLOAT:
        return 4;
    case PixelFormat::R32G32_SFLOAT:
        return 8;
    case PixelFormat::R32_UINT:
        return 4;
    case PixelFormat::R32_SINT:
        return 4;
    case PixelFormat::R32G32_UINT:
        return 8;
    case PixelFormat::R32G32B32A32_UINT:
        return 16;

    case PixelFormat::R8G8B8A8_UNORM:
    case PixelFormat::R8G8B8A8_SRGB:
    case PixelFormat::B8G8R8A8_UNORM:
    case PixelFormat::B8G8R8A8_SRGB:
    case PixelFormat::D32_SFLOAT:
    case PixelFormat::D24_UNORM_S8_UINT:
        return 4;
    case PixelFormat::R16G16B16A16_SFLOAT:
    case PixelFormat::D32_SFLOAT_S8_UINT:
        return 8;
    case PixelFormat::R32G32B32A32_SFLOAT:
        return 16;
    case PixelFormat::D16_UNORM:
        return 2;
    default:
        return 0;
    }
}

enum class ImageType
{
    Image2D,
    Cubemap
};

// Compute the number of mip levels for an image whose largest dimension is
// `size`, such that the smallest mip level is at least `minMipSize` pixels on
// its largest side.
//
//   size = 1024, minMipSize = 8  -> 8 mip levels (1024, 512, ..., 16, 8)
//   size = 32,   minMipSize = 32 -> 1 mip level
//
// `minMipSize` of 0 is treated as 1 (full mip chain down to 1x1).
inline uint32_t mipLevelsForMinSize(uint32_t size, uint32_t minMipSize)
{
    if (size == 0)
    {
        return 1;
    }
    if (minMipSize == 0)
    {
        minMipSize = 1;
    }
    uint32_t levels = 1;
    uint32_t current = size;
    while ((current >> 1) >= minMipSize)
    {
        current >>= 1;
        ++levels;
    }
    return levels;
}

// --- Image description for standalone image creation -------------------------

struct ImageDescription {
    Size2D size;
    PixelFormat format = PixelFormat::R8G8B8A8_UNORM;
    ImageUsage usage = ImageUsage::Sampled | ImageUsage::TransferDst;
    ImageType type = ImageType::Image2D;
    uint32_t mipLevels = 1;
    // Minimum mip level size, in pixels, on the largest image dimension.
    // When > 0, the number of mip levels is derived from `size` and this value
    // (see mipLevelsForMinSize) and overrides `mipLevels`. When 0, `mipLevels`
    // is used as-is.
    uint32_t minMipSize = 0;
    std::string debugName; // Debug label for validation layers / Xcode GPU Capture
};

// Resolve the effective mip level count for an image description: derived from
// `minMipSize` when it is set, otherwise the explicit `mipLevels` value.
inline uint32_t effectiveMipLevels(const ImageDescription& description)
{
    if (description.minMipSize > 0)
    {
        uint32_t maxDim = description.size.width > description.size.height
                              ? description.size.width
                              : description.size.height;
        return mipLevelsForMinSize(maxDim, description.minMipSize);
    }
    return description.mipLevels;
}

// --- Vertex attribute format --------------------------------------------------

enum class Format
{
    Undefined = 0,
    R32_SFLOAT,
    R32G32_SFLOAT,
    R32G32B32_SFLOAT,
    R32G32B32A32_SFLOAT
};

// --- Buffer usage flags -------------------------------------------------------

enum class BufferUsage : uint32_t
{
    None = 0,
    Vertex = 1 << 0,
    Index = 1 << 1,
    Uniform = 1 << 2,
    Storage = 1 << 3,
    TransferSrc = 1 << 4,
    TransferDst = 1 << 5,
    AccelerationStructureBuildInput = 1 << 6,
    ShaderDeviceAddress = 1 << 7,
    ShaderBindingTable = 1 << 8
};

inline BufferUsage operator|(BufferUsage a, BufferUsage b)
{
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline BufferUsage operator&(BufferUsage a, BufferUsage b)
{
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline BufferUsage& operator|=(BufferUsage& a, BufferUsage b)
{
    return a = a | b;
}

inline bool hasFlag(BufferUsage flags, BufferUsage flag)
{
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

// --- Vertex description -------------------------------------------------------

enum class VertexSemantic
{
    Position,
    Normal,
    Color,
    TexCoord0,
    TexCoord1,
    Tangent
};

enum class VertexInputRate
{
    Vertex,
    Instance
};

struct VertexAttributeDescription {
    uint32_t location = 0;
    uint32_t binding = 0;
    VertexSemantic semantic = VertexSemantic::Position;
    Format format = Format::R32G32B32_SFLOAT;
    uint32_t offset = 0;
};

struct VertexBufferDescription {
    uint32_t binding = 0;
    uint32_t stride = 0;
    VertexInputRate inputRate = VertexInputRate::Vertex;
    std::vector<VertexAttributeDescription> attributes;
};

enum class CullMode
{
    None,
    Front,
    Back
};
enum class FrontFace
{
    Clockwise,
    CounterClockwise
};

// --- Cubemap -----------------------------------------------------------------

enum class CubemapFace : uint32_t
{
    PositiveX = 0,
    NegativeX = 1,
    PositiveY = 2,
    NegativeY = 3,
    PositiveZ = 4,
    NegativeZ = 5
};

struct CubeMapDescription {
    uint32_t size = 512;
    PixelFormat format = PixelFormat::R16G16B16A16_SFLOAT;
    ImageUsage usage = ImageUsage::Sampled | ImageUsage::ColorAttachment | ImageUsage::TransferSrc | ImageUsage::TransferDst;
    uint32_t mipLevels = 1;
    // Minimum mip level size, in pixels. When > 0, the number of mip levels is
    // derived from `size` and this value (see mipLevelsForMinSize) and
    // overrides `mipLevels`. When 0, `mipLevels` is used as-is.
    uint32_t minMipSize = 0;
    std::string debugName;
};

}
}
