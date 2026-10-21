/*
 *    business grade graphic engine (bg2 engine)
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

#include <bg2e/db/texture_archive.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace bg2e::db {
namespace {
constexpr std::array<char, 8> Magic{'B','G','2','T','E','X','0','1'};

void writeInteger(std::ostream& out, uint64_t value, unsigned bytes)
{
    for (unsigned i = 0; i < bytes; ++i)
    {
        out.put(static_cast<char>(value >> (i * 8)));
    }
}

uint64_t readInteger(std::istream& input, unsigned bytes)
{
    uint64_t value = 0;
    for (unsigned i = 0; i < bytes; ++i)
    {
        value |= uint64_t(static_cast<unsigned char>(input.get())) << (i * 8);
    }
    return value;
}

size_t subresourceBytes(const gpu::ImageDescription& d, uint32_t mip)
{
    const size_t width = std::max(1u, d.size.width >> mip);
    const size_t height = std::max(1u, d.size.height >> mip);
    const size_t pixel = gpu::pixelFormatBytes(d.format);
    if (!pixel || width > std::numeric_limits<size_t>::max() / pixel / height)
    {
        throw std::runtime_error("Texture archive dimensions overflow");
    }
    return width * height * pixel;
}

void validate(const gpu::ImageDescription& d)
{
    if (std::endian::native != std::endian::little ||
        static_cast<uint32_t>(d.format) > static_cast<uint32_t>(gpu::PixelFormat::R32G32B32A32_UINT) ||
        gpu::isDepthFormat(d.format) ||
        (d.type != gpu::ImageType::Image2D && d.type != gpu::ImageType::Cubemap))
    {
        throw std::runtime_error("Unsupported texture archive format/type/byte order");
    }
    if (d.mipLevels == 0)
    {
        throw std::runtime_error("Texture archive requires an explicit mip count");
    }
    gpu::Image::validateDescription(d);
    subresourceBytes(d, 0);
}
}

TextureArchive readTextureArchive(const std::filesystem::path& path)
{
    std::ifstream input;
    input.exceptions(std::ios::badbit | std::ios::failbit);
    input.open(path, std::ios::binary);
    const auto fileBytes = std::filesystem::file_size(path);
    std::array<char, 8> magic{};
    input.read(magic.data(), magic.size());
    if (magic != Magic)
    {
        throw std::runtime_error("Unsupported texture archive: " + path.string());
    }
    TextureArchive result;
    auto& d = result.description;
    d.format = static_cast<gpu::PixelFormat>(readInteger(input, 4));
    d.type = static_cast<gpu::ImageType>(readInteger(input, 4));
    d.size.width = static_cast<uint32_t>(readInteger(input, 4));
    d.size.height = static_cast<uint32_t>(readInteger(input, 4));
    d.mipLevels = static_cast<uint32_t>(readInteger(input, 4));
    d.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst;
    validate(d);
    const uint32_t layers = d.type == gpu::ImageType::Cubemap ? 6 : 1;
    for (uint32_t layer = 0; layer < layers; ++layer)
    {
        for (uint32_t mip = 0; mip < d.mipLevels; ++mip)
        {
            const auto bytes = readInteger(input, 8);
            const auto position = static_cast<uintmax_t>(input.tellg());
            if (bytes != subresourceBytes(d, mip) || position > fileBytes || bytes > fileBytes - position ||
                bytes > static_cast<uint64_t>(std::numeric_limits<std::streamsize>::max()))
            {
                throw std::runtime_error("Invalid texture archive subresource length");
            }
            auto& data = result.subresources.emplace_back(static_cast<size_t>(bytes));
            input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
        }
    }
    if (static_cast<uintmax_t>(input.tellg()) != fileBytes)
    {
        throw std::runtime_error("Texture archive contains trailing bytes");
    }
    return result;
}

void writeTextureArchive(const std::filesystem::path& path, gpu::Image& image)
{
    gpu::ImageDescription d;
    d.size = image.size();
    d.format = image.pixelFormat();
    d.type = image.imageType();
    d.mipLevels = image.mipLevels();
    d.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst;
    validate(d);
    std::ofstream output;
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output.open(path, std::ios::binary);
    output.write(Magic.data(), Magic.size());
    writeInteger(output, static_cast<uint32_t>(d.format), 4);
    writeInteger(output, static_cast<uint32_t>(d.type), 4);
    writeInteger(output, d.size.width, 4);
    writeInteger(output, d.size.height, 4);
    writeInteger(output, d.mipLevels, 4);
    for (uint32_t layer = 0; layer < image.arrayLayers(); ++layer)
    {
        for (uint32_t mip = 0; mip < d.mipLevels; ++mip)
        {
            gpu::ImageSubresourceRange range{mip, 1, layer, 1};
            std::vector<uint8_t> bytes;
            image.readPixels(bytes, range, image.currentLayout(range));
            if (bytes.size() != subresourceBytes(d, mip) ||
                bytes.size() > static_cast<size_t>(std::numeric_limits<std::streamsize>::max()))
            {
                throw std::runtime_error("Unexpected texture archive readback length");
            }
            writeInteger(output, bytes.size(), 8);
            output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        }
    }
    output.close();
}

void writeTextureArchive(const std::filesystem::path& path, const TextureArchive& archive)
{
    const auto& d = archive.description;
    if (d.minMipSize != 0)
    {
        throw std::invalid_argument("Texture archive CPU description requires explicit mipLevels, not minMipSize");
    }
    validate(d);
    const uint32_t layers = d.type == gpu::ImageType::Cubemap ? 6 : 1;
    if (archive.subresources.size() != size_t(layers)*d.mipLevels)
    {
        throw std::invalid_argument("Texture archive CPU subresource count mismatch");
    }
    for (uint32_t layer = 0; layer < layers; ++layer)
    {
        for (uint32_t mip = 0; mip < d.mipLevels; ++mip)
        {
            const auto& bytes = archive.subresources[size_t(layer)*d.mipLevels+mip];
            if (bytes.size() != subresourceBytes(d,mip) ||
                bytes.size() > static_cast<size_t>(std::numeric_limits<std::streamsize>::max()))
            {
                throw std::invalid_argument("Texture archive CPU byte count mismatch");
            }
        }
    }
    std::ofstream output;
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output.open(path,std::ios::binary);
    output.write(Magic.data(),Magic.size());
    writeInteger(output,static_cast<uint32_t>(d.format),4);
    writeInteger(output,static_cast<uint32_t>(d.type),4);
    writeInteger(output,d.size.width,4);
    writeInteger(output,d.size.height,4);
    writeInteger(output,d.mipLevels,4);
    for (const auto& bytes : archive.subresources)
    {
        writeInteger(output,bytes.size(),8);
        output.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    }
    output.close();
}

}
