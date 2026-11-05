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

#include <bg2e/base/TextFont.hpp>
#include "TextDiagnostics.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <optional>
#include <string>
#include <utility>

// This independent MIT-option implementation must remain private to this TU.
// Upstream defines many unused static entry points; do not suppress warnings in
// repository-owned code or export symbols that could collide with ImGui's copy.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4505)
#endif
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace bg2e::base {
namespace {
uint32_t read32(const std::vector<uint8_t>& data, size_t offset, bool& valid)
{
    if (offset > data.size() || data.size() - offset < 4)
    {
        valid = false;
        return 0;
    }
    return (uint32_t(data[offset]) << 24) | (uint32_t(data[offset+1]) << 16) |
        (uint32_t(data[offset+2]) << 8) | uint32_t(data[offset+3]);
}


// These are structural/size checks, NOT a security validation of font contents.
std::optional<size_t> validateDirectory(const std::vector<uint8_t>& data, uint32_t& face)
{
    bool valid = true;
    size_t offset = 0;
    if (read32(data, 0, valid) == 0x74746366u) // ttcf
    {
        const auto count = read32(data, 8, valid);
        if (!count || count > TextFont::MaxFaces)
        {
            return std::nullopt;
        }
        if (face >= count)
        {
            detail::textWarning("Requested collection face is unavailable; using face 0.");
            face = 0;
        }
        offset = read32(data, 12 + size_t(face) * 4, valid);
    }
    else if (face != 0)
    {
        detail::textWarning("Standalone font has no requested face; using face 0.");
        face = 0;
    }
    const auto signature = read32(data, offset, valid);
    if (signature != 0x00010000u && signature != 0x4f54544fu &&
        signature != 0x74727565u && signature != 0x74797031u)
    {
        return std::nullopt;
    }
    if (!valid || offset > data.size() || data.size() - offset < 12)
    {
        return std::nullopt;
    }
    const auto tables = (uint32_t(data[offset+4]) << 8) | data[offset+5];
    if (!tables || tables > 4096 || tables > (data.size() - offset - 12) / 16)
    {
        return std::nullopt;
    }
    for (uint32_t i = 0; i < tables; ++i)
    {
        const auto entry = offset + 12 + size_t(i) * 16;
        const auto start = read32(data, entry+8, valid);
        const auto length = read32(data, entry+12, valid);
        if (start > data.size() || length > data.size() - start)
        {
            return std::nullopt;
        }
    }
    return valid ? std::optional<size_t>(offset) : std::nullopt;
}
}

struct TextFont::Impl {
    std::filesystem::path path;
    uint32_t face = 0;
    std::vector<uint8_t> bytes; // Stable storage for every pointer in stb state.
    stbtt_fontinfo info{};
    Metrics metrics;
};


TextFont::TextFont(std::unique_ptr<Impl> implementation) : _impl(std::move(implementation))
{
}


TextFont::~TextFont() = default;


std::shared_ptr<const TextFont> TextFont::load(const std::filesystem::path& path, uint32_t faceIndex) noexcept
try
{
    if (path.empty())
    {
        detail::textWarning("Empty font path; font unavailable.");
        return nullptr;
    }
    if (faceIndex >= MaxFaces)
    {
        detail::textWarning("Font face index exceeds the supported range; using face 0.");
        faceIndex = 0;
    }
    auto data = std::make_unique<Impl>();
    data->path = std::filesystem::absolute(path).lexically_normal();
    data->face = faceIndex;
    std::ifstream file(data->path, std::ios::binary | std::ios::ate);
    if (!file)
    {
        detail::textWarning("Cannot open font: ", data->path.string(), "; dependent text will be transparent.");
        return nullptr;
    }
    const auto length = file.tellg();
    if (length < std::streampos(12) || length > std::streampos(MaxFileBytes))
    {
        detail::textWarning("Font file size is outside [12 bytes,64 MiB]: ", data->path.string());
        return nullptr;
    }
    data->bytes.resize(static_cast<size_t>(length));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(data->bytes.data()), static_cast<std::streamsize>(data->bytes.size())))
    {
        detail::textWarning("Cannot read complete font: ", data->path.string());
        return nullptr;
    }
    const auto offset = validateDirectory(data->bytes, data->face);
    if (!offset || !stbtt_InitFont(&data->info, data->bytes.data(), static_cast<int>(*offset)) || data->info.numGlyphs <= 0)
    {
        detail::textWarning("Invalid/unsupported font directory or face: ", data->path.string());
        return nullptr;
    }
    stbtt_GetFontVMetrics(&data->info, &data->metrics.ascent, &data->metrics.descent, &data->metrics.lineGap);
    data->metrics.glyphCount = static_cast<uint32_t>(data->info.numGlyphs);
    if (data->metrics.ascent <= data->metrics.descent || data->metrics.glyphCount > 65535)
    {
        detail::textWarning("Invalid font metrics: ", data->path.string());
        return nullptr;
    }
    return std::shared_ptr<const TextFont>(new TextFont(std::move(data)));
}
catch (...)
{
    detail::textWarning("Cannot load font (I/O/allocation failure); font unavailable.");
    return nullptr;
}


const std::filesystem::path& TextFont::path() const
{
    return _impl->path;
}


uint32_t TextFont::faceIndex() const
{
    return _impl->face;
}


size_t TextFont::byteSize() const
{
    return _impl->bytes.size();
}


TextFont::Metrics TextFont::metrics() const
{
    return _impl->metrics;
}


uint32_t TextFont::checkedGlyph(uint32_t glyph) const noexcept
{
    if (glyph >= _impl->metrics.glyphCount)
    {
        detail::textWarning("Glyph index is out of range; using .notdef glyph 0.");
        return 0;
    }
    return glyph;
}


uint32_t TextFont::glyphIndex(char32_t codepoint) const noexcept
{
    if (codepoint > 0x10ffff || (codepoint >= 0xd800 && codepoint <= 0xdfff))
    {
        detail::textWarning("Invalid Unicode scalar; using U+FFFD.");
        codepoint = U'\ufffd';
    }
    const auto glyph = checkedGlyph(static_cast<uint32_t>(stbtt_FindGlyphIndex(&_impl->info, static_cast<int>(codepoint))));
    if (!glyph && codepoint != 0)
    {
        detail::textWarning("Codepoint ", static_cast<uint32_t>(codepoint), " is absent from the font; using .notdef.");
    }
    return glyph;
}


TextFont::GlyphMetrics TextFont::glyphMetrics(uint32_t glyph) const noexcept
{
    glyph = checkedGlyph(glyph);
    GlyphMetrics result;
    stbtt_GetGlyphHMetrics(&_impl->info, static_cast<int>(glyph), &result.advanceWidth, &result.leftSideBearing);
    stbtt_GetGlyphBox(&_impl->info, static_cast<int>(glyph), &result.x0, &result.y0, &result.x1, &result.y1);
    return result;
}


int TextFont::kerningAdvance(uint32_t leftGlyph, uint32_t rightGlyph) const noexcept
{
    leftGlyph = checkedGlyph(leftGlyph);
    rightGlyph = checkedGlyph(rightGlyph);
    return stbtt_GetGlyphKernAdvance(&_impl->info, static_cast<int>(leftGlyph), static_cast<int>(rightGlyph));
}


float TextFont::scaleForPixelHeight(float pixelHeight) const noexcept
{
    if (!std::isfinite(pixelHeight) || pixelHeight < 1 || pixelHeight > MaxGlyphBitmapDimension)
    {
        detail::textWarning("Invalid font pixel height; applying defaults/limits.");
        pixelHeight = std::isfinite(pixelHeight) && pixelHeight > 0 ? std::clamp(pixelHeight, 1.0f, 4096.0f) : 32;
    }
    return stbtt_ScaleForPixelHeight(&_impl->info, pixelHeight);
}


TextFont::GlyphBitmap TextFont::rasterizeGlyph(uint32_t glyph, float pixelHeight, float shiftX, float shiftY) const noexcept
{
    const auto scale = scaleForPixelHeight(pixelHeight);
    return rasterizeGlyphScaled(glyph, scale, scale, shiftX, shiftY);
}


TextFont::GlyphBitmap TextFont::rasterizeGlyphScaled(uint32_t glyph, float scaleX, float scaleY,
    float shiftX, float shiftY) const noexcept
try
{
    glyph = checkedGlyph(glyph);
    const double fontHeight = double(_impl->metrics.ascent) - _impl->metrics.descent;
    if (!std::isfinite(scaleX) || !std::isfinite(scaleY) || scaleX <= 0 || scaleY <= 0 ||
        scaleX * fontHeight > MaxGlyphBitmapDimension + 0.001 || scaleY * fontHeight > MaxGlyphBitmapDimension + 0.001)
    {
        detail::textWarning("Invalid font raster scales; applying defaults/limits.");
        const float fallback = scaleForPixelHeight(32);
        const float maximum = static_cast<float>(MaxGlyphBitmapDimension / fontHeight);
        scaleX = std::isfinite(scaleX) && scaleX > 0 ? std::min(scaleX, maximum) : fallback;
        scaleY = std::isfinite(scaleY) && scaleY > 0 ? std::min(scaleY, maximum) : fallback;
    }
    if (!std::isfinite(shiftX) || !std::isfinite(shiftY) || shiftX < 0 || shiftX >= 1 || shiftY < 0 || shiftY >= 1)
    {
        detail::textWarning("Invalid font subpixel offset; using zero offset.");
        shiftX = shiftY = 0;
    }
    GlyphBitmap result;
    int x1 = 0, y1 = 0;
    stbtt_GetGlyphBitmapBoxSubpixel(&_impl->info, static_cast<int>(glyph), scaleX, scaleY, shiftX, shiftY,
        &result.xOffset, &result.yOffset, &x1, &y1);
    const auto width = int64_t(x1) - result.xOffset;
    const auto height = int64_t(y1) - result.yOffset;
    if (width < 0 || height < 0 || width > MaxGlyphBitmapDimension || height > MaxGlyphBitmapDimension)
    {
        detail::textWarning("Glyph bitmap exceeds dimension limits; glyph unavailable.");
        return {.valid = false};
    }
    result.width = static_cast<int>(width);
    result.height = static_cast<int>(height);
    if (width && height)
    {
        result.coverage.resize(size_t(width) * size_t(height));
        stbtt_MakeGlyphBitmapSubpixel(&_impl->info, result.coverage.data(), result.width, result.height,
            result.width, scaleX, scaleY, shiftX, shiftY, static_cast<int>(glyph));
    }
    return result;
}
catch (...)
{
    detail::textWarning("Cannot allocate/rasterize glyph; glyph unavailable.");
    return {.valid = false};
}

}
