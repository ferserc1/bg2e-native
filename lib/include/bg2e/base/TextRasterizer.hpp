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

#include <bg2e/base/Text.hpp>
#include <bg2e/base/TextFont.hpp>
#include <vector>

namespace bg2e::base {

struct TextLayout {
    struct Glyph {
        uint32_t index = 0;
        double x = 0, baseline = 0; // Logical units, top-left origin, Y downward.
    };
    struct Line {
        size_t firstGlyph = 0, glyphCount = 0;
        double width = 0, baseline = 0;
    };
    std::vector<Glyph> glyphs;
    std::vector<Line> lines;
    double contentWidth = 0, contentHeight = 0;
    bool overflowed = false;
    bool renderable = true; // False means transparent/omitted, never an exception.
};

struct TextRasterization {
    uint32_t width = 0, height = 0;
    TextLayout layout;
    std::vector<uint8_t> rgba; // White RGB even in transparent texels; alpha coverage.
};

// Explicitly resolved immutable font; no registry, filesystem or GPU access.
// Initial support: simple LTR scalar layout and kerning, not shaping or bidi.
// CR/LF/CRLF are line breaks, tabs expand to four spaces. Word wrapping breaks
// at ASCII spaces and falls back to glyph breaks for oversized words. Trailing
// breakable spaces are trimmed; explicit leading spaces are otherwise retained.
// Clip keeps ink inside margins; Hide omits overflowing text with a warning.
// Recoverable failures never escape; allocation failure may return an empty
// Rasterization, which callers must treat as unavailable rather than upload.
class BG2E_API TextRasterizer {
  public:
    static constexpr size_t MaxGlyphs = 65536; // Includes expanded tab spaces/newlines.
    static constexpr size_t MaxRasterizedGlyphPixels = 64 * 1024 * 1024;

    static TextLayout layout(const Text& text, const TextFont& font) noexcept;
    static TextRasterization rasterize(const Text& text, const TextFont& font) noexcept;
};

}
