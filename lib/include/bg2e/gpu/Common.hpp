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

struct PushConstantRange {
    uint32_t    offset = 0;
    uint32_t    size   = 0;     // Vulkan minimum guaranteed: 128 bytes; Metal limit: ~4 KB
    ShaderStage stage  = ShaderStage::Vertex; // single shader stage mapping (upgrade to bitmask later)
};

// --- Resource binding types ---------------------------------------------------

enum class ResourceType {
    UniformBuffer,   // reserved, not implemented this iteration
    StorageBuffer,   // reserved, not implemented this iteration
    SampledImage,    // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE / MTL texture binding
    StorageImage,    // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE / MTL texture binding (read-write)
    Sampler          // VK_DESCRIPTOR_TYPE_SAMPLER / MTL sampler binding
};

struct ResourceBinding {
    uint32_t     set     = 0;
    uint32_t     binding = 0;
    ResourceType type    = ResourceType::SampledImage;
    ShaderStage  stage   = ShaderStage::Fragment;
    uint32_t     count   = 1;   // array size; 1 for this iteration
};

struct PipelineLayoutDescription {
    std::vector<PushConstantRange> pushConstants; // empty for the first triangle pipeline
    std::vector<ResourceBinding>   resourceBindings; // empty == no sets
    std::string debugName;           // Debug label for validation layers / Xcode GPU Capture
};

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

// --- Image description for standalone image creation -------------------------

struct ImageDescription {
    Size2D      size;
    PixelFormat format = PixelFormat::R8G8B8A8_UNORM;
    ImageUsage  usage  = ImageUsage::Sampled | ImageUsage::TransferDst;
    std::string debugName;          // Debug label for validation layers / Xcode GPU Capture
};

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

}
}
