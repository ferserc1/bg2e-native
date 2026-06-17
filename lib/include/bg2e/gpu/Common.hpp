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

#include <bg2e/common.hpp>

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
    uint32_t width  = 0;
    uint32_t height = 0;

    Size2D() = default;
    Size2D(uint32_t w, uint32_t h) : width(w), height(h) {}

    bool operator==(const Size2D& o) const { return width == o.width && height == o.height; }
    bool operator!=(const Size2D& o) const { return !(*this == o); }
    bool isZero() const { return width == 0 || height == 0; }
};

struct Size3D {
    uint32_t width  = 0;
    uint32_t height = 0;
    uint32_t depth  = 1;

    Size3D() = default;
    Size3D(uint32_t w, uint32_t h, uint32_t d = 1) : width(w), height(h), depth(d) {}
    explicit Size3D(const Size2D& s, uint32_t d = 1) : width(s.width), height(s.height), depth(d) {}

    Size2D toSize2D() const { return Size2D{ width, height }; }

    bool operator==(const Size3D& o) const { return width == o.width && height == o.height && depth == o.depth; }
    bool operator!=(const Size3D& o) const { return !(*this == o); }
};

enum class PixelFormat {
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
    D32_SFLOAT_S8_UINT
};

constexpr bool isDepthFormat(PixelFormat f)
{
    return f == PixelFormat::D16_UNORM
        || f == PixelFormat::D32_SFLOAT
        || f == PixelFormat::D24_UNORM_S8_UINT
        || f == PixelFormat::D32_SFLOAT_S8_UINT;
}

constexpr bool hasStencil(PixelFormat f)
{
    return f == PixelFormat::D24_UNORM_S8_UINT
        || f == PixelFormat::D32_SFLOAT_S8_UINT;
}

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    Color() = default;
    Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}
};

enum class ImageLayout {
    Undefined = 0,
    General,
    ColorAttachment,
    DepthAttachment,
    ShaderReadOnly,
    TransferSrc,
    TransferDst,
    Present
};

enum class ShaderStage {
    Vertex,
    Fragment,
    Compute
};

enum class PipelineBarrierFlags : uint32_t {
    None                 = 0,
    AllCommands          = 1 << 0,
    VertexInput          = 1 << 1,
    VertexShader         = 1 << 2,
    FragmentShader       = 1 << 3,
    ComputeShader        = 1 << 4,
    Transfer             = 1 << 5,
    ColorAttachmentOutput = 1 << 6,
    EarlyFragment        = 1 << 7,
    LateFragment         = 1 << 8,
    BottomOfPipe         = 1 << 9,
    TopOfPipe            = 1 << 10,
    MemoryRead           = 1 << 11,
    MemoryWrite          = 1 << 12
};

inline PipelineBarrierFlags operator|(PipelineBarrierFlags a, PipelineBarrierFlags b)
{
    return static_cast<PipelineBarrierFlags>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
    );
}

inline PipelineBarrierFlags operator&(PipelineBarrierFlags a, PipelineBarrierFlags b)
{
    return static_cast<PipelineBarrierFlags>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
    );
}

inline PipelineBarrierFlags& operator|=(PipelineBarrierFlags& a, PipelineBarrierFlags b)
{
    return a = a | b;
}

inline bool hasFlag(PipelineBarrierFlags flags, PipelineBarrierFlags flag)
{
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

struct ShaderModuleDescription {
    std::string filePath;            // Vulkan: .spv ; Metal: .metallib
    std::string entryPoint = "main"; // Vulkan: SPIR-V entry ; Metal: MTL function name
    ShaderStage stage = ShaderStage::Vertex;
    std::string debugName;           // Debug label for validation layers / Xcode GPU Capture
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
    uint32_t    offset = 0;     // Vulkan push constant offset (ignored by Metal)
    uint32_t    size   = 0;     // Vulkan minimum guaranteed: 128 bytes; Metal limit: ~4 KB
    ShaderStage stage  = ShaderStage::Vertex; // single shader stage (not a bitmask)
};

// --- Resource binding types ---------------------------------------------------

enum class ResourceType {
    UniformBuffer,   // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER / Metal [[buffer(N)]]
    StorageBuffer,   // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER / Metal [[buffer(N)]]
    SampledImage,    // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE  / Metal [[texture(N)]]
    StorageImage,    // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE  / Metal [[texture(N)]] (read-write)
    Sampler          // VK_DESCRIPTOR_TYPE_SAMPLER        / Metal [[sampler(N)]]
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
    uint32_t metal  = 0;
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
    uint32_t      set     = 0;
    ShaderBinding binding = {};
    ResourceType  type    = ResourceType::SampledImage;
    ShaderStage   stage   = ShaderStage::Fragment;
    uint32_t      count   = 1;   // array size; 1 for this iteration
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
    std::vector<ResourceBinding>   resourceBindings;
    std::string debugName;           // Debug label for validation layers / Xcode GPU Capture
};

// Validate that at most one PushConstantRange is declared per ShaderStage.
// Throws std::runtime_error on failure.
inline void validatePushConstantRanges(const std::vector<PushConstantRange>& ranges)
{
    bool hasVertex = false;
    bool hasFragment = false;
    bool hasCompute = false;
    for (const auto& r : ranges)
    {
        switch (r.stage)
        {
            case ShaderStage::Vertex:
                if (hasVertex)
                {
                    throw std::runtime_error(
                        "PipelineLayout validation failed: more than one push constant range "
                        "declared for Vertex stage.");
                }
                hasVertex = true;
                break;
            case ShaderStage::Fragment:
                if (hasFragment)
                {
                    throw std::runtime_error(
                        "PipelineLayout validation failed: more than one push constant range "
                        "declared for Fragment stage.");
                }
                hasFragment = true;
                break;
            case ShaderStage::Compute:
                if (hasCompute)
                {
                    throw std::runtime_error(
                        "PipelineLayout validation failed: more than one push constant range "
                        "declared for Compute stage.");
                }
                hasCompute = true;
                break;
        }
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
                             rb.type == ResourceType::StorageBuffer);
        if (!isBufferType) continue;

        switch (rb.stage)
        {
            case ShaderStage::Vertex:
                if (rb.binding.metal < 2)
                {
                    throw std::runtime_error(
                        "PipelineLayout validation failed: Metal vertex buffer resource binding "
                        "uses index " + std::to_string(rb.binding.metal) + ", but vertex-stage "
                        "UniformBuffer/StorageBuffer bindings must use index >= 2. buffer(0) is "
                        "reserved for the geometric vertex buffer and buffer(1) is reserved for "
                        "vertex push constants.");
                }
                break;
            case ShaderStage::Fragment:
                if (rb.binding.metal < 1)
                {
                    throw std::runtime_error(
                        "PipelineLayout validation failed: Metal fragment buffer resource binding "
                        "uses index " + std::to_string(rb.binding.metal) + ", but fragment-stage "
                        "UniformBuffer/StorageBuffer bindings must use index >= 1. buffer(0) is "
                        "reserved for fragment push constants.");
                }
                break;
            case ShaderStage::Compute:
                if (rb.binding.metal < 1)
                {
                    throw std::runtime_error(
                        "PipelineLayout validation failed: Metal compute buffer resource binding "
                        "uses index " + std::to_string(rb.binding.metal) + ", but compute-stage "
                        "UniformBuffer/StorageBuffer bindings must use index >= 1. buffer(0) is "
                        "reserved for compute push constants.");
                }
                break;
        }
    }
}

// --- Sampler description vocabulary ------------------------------------------

enum class Filter       { Nearest, Linear };
enum class MipmapFilter { Nearest, Linear };
enum class AddressMode  { Repeat, ClampToEdge, MirroredRepeat };

struct SamplerDescription {
    Filter       minFilter    = Filter::Linear;
    Filter       magFilter    = Filter::Linear;
    MipmapFilter mipFilter    = MipmapFilter::Nearest;
    AddressMode  addressModeU = AddressMode::Repeat;
    AddressMode  addressModeV = AddressMode::Repeat;
    AddressMode  addressModeW = AddressMode::Repeat;
    std::string  debugName;          // Debug label for validation layers / Xcode GPU Capture
};

// --- Image usage flags -------------------------------------------------------

enum class ImageUsage : uint32_t {
    None            = 0,
    ColorAttachment = 1 << 0,
    DepthStencil    = 1 << 1,
    Sampled         = 1 << 2,
    Storage         = 1 << 3,
    TransferSrc     = 1 << 4,
    TransferDst     = 1 << 5,
    Present         = 1 << 6
};

inline ImageUsage operator|(ImageUsage a, ImageUsage b)
{
    return static_cast<ImageUsage>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
    );
}

inline ImageUsage operator&(ImageUsage a, ImageUsage b)
{
    return static_cast<ImageUsage>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
    );
}

inline ImageUsage& operator|=(ImageUsage& a, ImageUsage b)
{
    return a = a | b;
}

inline bool hasFlag(ImageUsage flags, ImageUsage flag)
{
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

enum class ImageType {
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
    if (size == 0) return 1;
    if (minMipSize == 0) minMipSize = 1;
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
    Size2D      size;
    PixelFormat format = PixelFormat::R8G8B8A8_UNORM;
    ImageUsage  usage  = ImageUsage::Sampled | ImageUsage::TransferDst;
    ImageType   type   = ImageType::Image2D;
    uint32_t    mipLevels = 1;
    // Minimum mip level size, in pixels, on the largest image dimension.
    // When > 0, the number of mip levels is derived from `size` and this value
    // (see mipLevelsForMinSize) and overrides `mipLevels`. When 0, `mipLevels`
    // is used as-is.
    uint32_t    minMipSize = 0;
    std::string debugName;          // Debug label for validation layers / Xcode GPU Capture
};

// Resolve the effective mip level count for an image description: derived from
// `minMipSize` when it is set, otherwise the explicit `mipLevels` value.
inline uint32_t effectiveMipLevels(const ImageDescription& description)
{
    if (description.minMipSize > 0)
    {
        uint32_t maxDim = description.size.width > description.size.height
            ? description.size.width : description.size.height;
        return mipLevelsForMinSize(maxDim, description.minMipSize);
    }
    return description.mipLevels;
}

// --- Vertex attribute format --------------------------------------------------

enum class Format {
    Undefined = 0,
    R32_SFLOAT,
    R32G32_SFLOAT,
    R32G32B32_SFLOAT,
    R32G32B32A32_SFLOAT
};

// --- Buffer usage flags -------------------------------------------------------

enum class BufferUsage : uint32_t {
    None                            = 0,
    Vertex                          = 1 << 0,
    Index                           = 1 << 1,
    Uniform                         = 1 << 2,
    Storage                         = 1 << 3,
    TransferSrc                     = 1 << 4,
    TransferDst                     = 1 << 5,
    AccelerationStructureBuildInput = 1 << 6,
    ShaderDeviceAddress             = 1 << 7
};

inline BufferUsage operator|(BufferUsage a, BufferUsage b)
{
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline BufferUsage operator&(BufferUsage a, BufferUsage b)
{
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline BufferUsage& operator|=(BufferUsage& a, BufferUsage b) { return a = a | b; }

inline bool hasFlag(BufferUsage flags, BufferUsage flag)
{
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

// --- Vertex description -------------------------------------------------------

enum class VertexSemantic {
    Position,
    Normal,
    Color,
    TexCoord0,
    TexCoord1,
    Tangent
};

enum class VertexInputRate {
    Vertex,
    Instance
};

struct VertexAttributeDescription {
    uint32_t       location = 0;
    uint32_t       binding  = 0;
    VertexSemantic semantic = VertexSemantic::Position;
    Format         format   = Format::R32G32B32_SFLOAT;
    uint32_t       offset   = 0;
};

struct VertexBufferDescription {
    uint32_t                              binding   = 0;
    uint32_t                              stride    = 0;
    VertexInputRate                       inputRate = VertexInputRate::Vertex;
    std::vector<VertexAttributeDescription> attributes;
};

enum class CullMode { None, Front, Back };
enum class FrontFace { Clockwise, CounterClockwise };

// --- Cubemap -----------------------------------------------------------------

enum class CubemapFace : uint32_t {
    PositiveX = 0,
    NegativeX = 1,
    PositiveY = 2,
    NegativeY = 3,
    PositiveZ = 4,
    NegativeZ = 5
};

struct CubeMapDescription {
    uint32_t    size   = 512;
    PixelFormat format = PixelFormat::R16G16B16A16_SFLOAT;
    ImageUsage  usage  = ImageUsage::Sampled
                       | ImageUsage::ColorAttachment
                       | ImageUsage::TransferSrc
                       | ImageUsage::TransferDst;
    uint32_t    mipLevels = 1;
    // Minimum mip level size, in pixels. When > 0, the number of mip levels is
    // derived from `size` and this value (see mipLevelsForMinSize) and
    // overrides `mipLevels`. When 0, `mipLevels` is used as-is.
    uint32_t    minMipSize = 0;
    std::string debugName;
};

}
}
