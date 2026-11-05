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

#pragma once

#include <bg2e/common.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

namespace bg2e::base {

// Immutable CPU font resource. Trusted project fonts only: the underlying stb
// parser is not safe for hostile files. Public types never expose stb state.
class BG2E_API TextFont final {
  public:
    static constexpr size_t MaxFileBytes = 64 * 1024 * 1024;
    static constexpr uint32_t MaxFaces = 32;
    static constexpr uint32_t MaxGlyphBitmapDimension = 4096;

    struct Metrics {
        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
        uint32_t glyphCount = 0;
    };
    struct GlyphMetrics {
        int advanceWidth = 0;
        int leftSideBearing = 0;
        int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    };
    struct GlyphBitmap {
        int width = 0, height = 0;
        int xOffset = 0, yOffset = 0;
        std::vector<uint8_t> coverage; // Row-major, top-to-bottom, one alpha byte.
        bool valid = true; // False means unavailable, not an empty whitespace glyph.
    };

    // Unavailable/invalid fonts warn and return nullptr; no text error escapes.
    static std::shared_ptr<const TextFont> load(const std::filesystem::path& path, uint32_t faceIndex = 0) noexcept;
    ~TextFont();
    TextFont(const TextFont&) = delete;
    TextFont& operator=(const TextFont&) = delete;

    const std::filesystem::path& path() const;
    uint32_t faceIndex() const;
    size_t byteSize() const;
    Metrics metrics() const; // Unscaled font units.

    // Unicode scalar lookup, without shaping/bidi/fallback chains. Missing
    // characters return glyph 0 (.notdef), not a font-registry fallback.
    uint32_t glyphIndex(char32_t codepoint) const noexcept;
    GlyphMetrics glyphMetrics(uint32_t glyph) const noexcept;
    int kerningAdvance(uint32_t leftGlyph, uint32_t rightGlyph) const noexcept;
    float scaleForPixelHeight(float pixelHeight) const noexcept;
    GlyphBitmap rasterizeGlyph(uint32_t glyph, float pixelHeight,
        float shiftX = 0, float shiftY = 0) const noexcept;
    // Independent bitmap-axis densities do not affect logical paragraph layout.
    // Scales are pixels per unscaled font unit; subpixel glyphs are supported.
    GlyphBitmap rasterizeGlyphScaled(uint32_t glyph, float scaleX, float scaleY,
        float shiftX = 0, float shiftY = 0) const noexcept;

  private:
    struct Impl;
    explicit TextFont(std::unique_ptr<Impl> implementation);
    std::unique_ptr<Impl> _impl;

    uint32_t checkedGlyph(uint32_t glyph) const noexcept;
};

}
