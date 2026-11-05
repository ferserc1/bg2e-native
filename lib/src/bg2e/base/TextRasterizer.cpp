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

#include <bg2e/base/TextRasterizer.hpp>
#include "TextUtf8.hpp"
#include "TextDiagnostics.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace bg2e::base {
namespace {
struct Token {
    uint32_t glyph = 0;
    bool space = false, newline = false;
    TextFont::GlyphMetrics metrics;
};


std::vector<Token> tokens(const Text& text, const TextFont& font, bool& truncated)
{
    std::vector<Token> result;
    std::unordered_map<char32_t, Token> cache;
    const auto append = [&](char32_t scalar)
    {
        if (result.size() >= TextRasterizer::MaxGlyphs)
        {
            detail::textWarning("Expanded glyph/line-break limit reached; remaining text omitted.");
            truncated = true;
            return false;
        }
        auto found = cache.find(scalar);
        if (found == cache.end())
        {
            Token token;
            token.space = scalar == U' ';
            token.newline = scalar == U'\n';
            if (!token.newline)
            {
                token.glyph = font.glyphIndex(scalar);
                token.metrics = font.glyphMetrics(token.glyph);
            }
            found = cache.emplace(scalar, token).first;
        }
        result.push_back(found->second);
        return true;
    };
    const auto& content = text.content();
    for (size_t i = 0; i < content.size();)
    {
        auto scalar = detail::nextTextScalar(content, i);
        if (scalar == U'\r')
        {
            if (i < content.size() && content[i] == '\n')
            {
                ++i;
            }
            scalar = U'\n';
        }
        if (scalar == U'\t')
        {
            for (int space = 0; space < 4; ++space)
            {
                if (!append(U' '))
                {
                    return result;
                }
            }
        }
        else
        {
            if ((scalar < 0x20 && scalar != U'\n') || (scalar >= 0x7f && scalar <= 0x9f))
            {
                scalar = U'\ufffd';
            }
            if (!append(scalar))
            {
                return result;
            }
        }
    }
    return result;
}

struct Measure {
    double pen = 0, minX = 0, maxX = 0;
    double minY = 0, maxY = 0;
    uint32_t previous = 0;
    bool hasPrevious = false;
};


double appendGlyph(Measure& state, const Token& token, const Text& text, const TextFont& font, double scale)
{
    double x = state.pen;
    if (state.hasPrevious)
    {
        x += font.kerningAdvance(state.previous, token.glyph) * scale + text.characterSpacing();
    }
    state.pen = x + token.metrics.advanceWidth * scale;
    state.minX = std::min(state.minX, state.pen);
    state.maxX = std::max(state.maxX, state.pen);
    if (token.metrics.x1 > token.metrics.x0 && token.metrics.y1 > token.metrics.y0)
    {
        state.minX = std::min(state.minX, x + token.metrics.x0 * scale);
        state.maxX = std::max(state.maxX, x + token.metrics.x1 * scale);
        state.minY = std::min(state.minY, -token.metrics.y1 * scale);
        state.maxY = std::max(state.maxY, -token.metrics.y0 * scale);
    }
    state.previous = token.glyph;
    state.hasPrevious = true;
    return x;
}

struct PixelOrigin {
    int64_t integral = 0;
    float fraction = 0;
    bool valid = true;
};


PixelOrigin pixelOrigin(double value)
{
    // Validated paragraph limits keep coordinates far inside int64. Check before
    // converting even for completely offscreen/negative character advances.
    if (!std::isfinite(value) || std::abs(value) > 1e15)
    {
        detail::textWarning("Raster coordinate exceeds supported range; glyph omitted.");
        return {.valid = false};
    }
    const auto integer = std::floor(value);
    PixelOrigin result{static_cast<int64_t>(integer), static_cast<float>(value - integer)};
    if (result.fraction >= 1)
    {
        ++result.integral;
        result.fraction = 0;
    }
    return result;
}
}


TextLayout TextRasterizer::layout(const Text& text, const TextFont& font) noexcept
try
{
    bool truncated = false;
    const auto input = tokens(text, font, truncated);
    TextLayout result;
    result.overflowed = truncated;
    if (input.empty())
    {
        return result;
    }
    const double scale = font.scaleForPixelHeight(text.fontSize());
    const auto metrics = font.metrics();
    const double ascent = metrics.ascent * scale, descent = metrics.descent * scale;
    const double advance = (double(metrics.ascent) - metrics.descent + std::max(metrics.lineGap, 0)) * scale * text.lineSpacing();
    const auto size = text.paragraphSize();
    const auto margins = text.margins();
    const double availableWidth = double(size.width) - margins.left - margins.right;
    const double availableHeight = double(size.height) - margins.top - margins.bottom;
    const auto initialMeasure = [&]()
    {
        Measure value;
        value.minY = -ascent;
        value.maxY = -descent;
        return value;
    };
    const auto tooWide = [&](const Measure& measure)
    {
        return measure.maxX - measure.minX > availableWidth + 1e-6 * std::max(1.0, availableWidth);
    };
    std::vector<Token> current;
    Measure state = initialMeasure();
    double minY = 0, maxY = 0;
    const auto finishLine = [&]()
    {
        // Breakable trailing whitespace is not part of visual line width.
        while (!current.empty() && current.back().space)
        {
            current.pop_back();
        }
        Measure measure = initialMeasure();
        const double baseline = double(result.lines.size()) * advance;
        const auto first = result.glyphs.size();
        for (const auto& token : current)
        {
            const auto x = appendGlyph(measure, token, text, font, scale);
            result.glyphs.push_back({token.glyph, x, baseline});
        }
        const double width = measure.maxX - measure.minX;
        const double align = text.alignment() == Text::Alignment::Left ? 0 :
            text.alignment() == Text::Alignment::Center ? 0.5 : 1;
        const double origin = margins.left - measure.minX + align * (availableWidth - width);
        for (size_t i = first; i < result.glyphs.size(); ++i)
        {
            result.glyphs[i].x += origin;
        }
        if (result.lines.empty())
        {
            minY = baseline + measure.minY;
            maxY = baseline + measure.maxY;
        }
        else
        {
            minY = std::min(minY, baseline + measure.minY);
            maxY = std::max(maxY, baseline + measure.maxY);
        }
        result.lines.push_back({first, result.glyphs.size() - first, width, baseline});
        result.contentWidth = std::max(result.contentWidth, width);
        result.overflowed = result.overflowed || tooWide(measure);
        current.clear();
        state = initialMeasure();
    };
    for (size_t i = 0; i < input.size();)
    {
        if (input[i].newline)
        {
            finishLine();
            ++i;
            continue;
        }
        if (input[i].space)
        {
            appendGlyph(state, input[i], text, font, scale);
            current.push_back(input[i]);
            ++i;
            continue;
        }
        size_t end = i + 1;
        while (end < input.size() && !input[end].space && !input[end].newline)
        {
            ++end;
        }
        if (text.wrapMode() == Text::WrapMode::Word && !current.empty())
        {
            auto proposed = state;
            for (auto j = i; j < end; ++j)
            {
                appendGlyph(proposed, input[j], text, font, scale);
            }
            if (tooWide(proposed))
            {
                finishLine();
            }
        }
        for (; i < end; ++i)
        {
            auto proposed = state;
            appendGlyph(proposed, input[i], text, font, scale);
            if (text.wrapMode() != Text::WrapMode::None && !current.empty() && tooWide(proposed))
            {
                finishLine();
                proposed = state;
                appendGlyph(proposed, input[i], text, font, scale);
            }
            state = proposed;
            current.push_back(input[i]);
        }
    }
    finishLine();
    result.contentHeight = maxY - minY;
    result.overflowed = result.overflowed || result.contentHeight > availableHeight + 1e-6 * std::max(1.0, availableHeight);
    const double align = text.verticalAlignment() == Text::VerticalAlignment::Top ? 0 :
        text.verticalAlignment() == Text::VerticalAlignment::Center ? 0.5 : 1;
    const double origin = margins.top - minY + align * (availableHeight - result.contentHeight);
    for (auto& glyph : result.glyphs)
    {
        glyph.baseline += origin;
    }
    for (auto& line : result.lines)
    {
        line.baseline += origin;
    }
    if (result.overflowed)
    {
        result.renderable = text.overflowPolicy() == Text::OverflowPolicy::Clip;
        detail::textWarning(result.renderable ? "Paragraph overflow; clipping to the margin interior." :
            "Paragraph overflow; text hidden by its overflow policy.");
    }
    return result;
}
catch (...)
{
    detail::textWarning("Cannot lay out paragraph; text omitted.");
    TextLayout result;
    result.renderable = false;
    return result;
}


TextRasterization TextRasterizer::rasterize(const Text& text, const TextFont& font) noexcept
try
{
    TextRasterization result;
    result.layout = layout(text, font);
    const auto resolution = text.bitmapResolution();
    result.width = resolution.width;
    result.height = resolution.height;
    result.rgba.assign(size_t(result.width) * result.height * 4, 255);
    for (size_t alpha = 3; alpha < result.rgba.size(); alpha += 4)
    {
        result.rgba[alpha] = 0;
    }
    if (!result.layout.renderable)
    {
        return result;
    }
    const auto size = text.paragraphSize();
    const auto margins = text.margins();
    const double sx = double(result.width) / size.width, sy = double(result.height) / size.height;
    const double fontScale = font.scaleForPixelHeight(text.fontSize());
    // Tiny tolerance prevents exact integer edges losing a pixel to roundoff.
    // Only pixels fully inside the margin rectangle are eligible for coverage.
    const auto left = std::clamp<int64_t>(static_cast<int64_t>(std::ceil(margins.left * sx - 1e-9)), 0, result.width);
    const auto top = std::clamp<int64_t>(static_cast<int64_t>(std::ceil(margins.top * sy - 1e-9)), 0, result.height);
    const auto right = std::clamp<int64_t>(static_cast<int64_t>(std::floor((double(size.width) - margins.right) * sx + 1e-9)), 0, result.width);
    const auto bottom = std::clamp<int64_t>(static_cast<int64_t>(std::floor((double(size.height) - margins.bottom) * sy + 1e-9)), 0, result.height);
    if (left >= right || top >= bottom)
    {
        return result;
    }
    size_t work = 0;
    for (const auto& glyph : result.layout.glyphs)
    {
        const auto bounds = font.glyphMetrics(glyph.index);
        if (bounds.x0 == bounds.x1 || bounds.y0 == bounds.y1 ||
            glyph.x + bounds.x1 * fontScale < margins.left - 1 / sx ||
            glyph.x + bounds.x0 * fontScale > size.width - margins.right + 1 / sx ||
            glyph.baseline - bounds.y0 * fontScale < margins.top - 1 / sy ||
            glyph.baseline - bounds.y1 * fontScale > size.height - margins.bottom + 1 / sy)
        {
            continue;
        }
        const auto x = pixelOrigin(glyph.x * sx), y = pixelOrigin(glyph.baseline * sy);
        if (!x.valid || !y.valid)
        {
            continue;
        }
        const auto mask = font.rasterizeGlyphScaled(glyph.index, static_cast<float>(fontScale * sx),
            static_cast<float>(fontScale * sy), x.fraction, y.fraction);
        if (!mask.valid || mask.coverage.size() > MaxRasterizedGlyphPixels - work)
        {
            detail::textWarning("Glyph raster unavailable or paragraph work budget exceeded; returning transparent text.");
            result.layout.renderable = false;
            for (size_t alpha = 3; alpha < result.rgba.size(); alpha += 4)
            {
                result.rgba[alpha] = 0;
            }
            return result;
        }
        work += mask.coverage.size();
        const int64_t x0 = x.integral + mask.xOffset, y0 = y.integral + mask.yOffset;
        const auto minX = std::max(left, x0), minY = std::max(top, y0);
        const auto maxX = std::min(right, x0 + mask.width), maxY = std::min(bottom, y0 + mask.height);
        for (auto row = minY; row < maxY; ++row)
        {
            for (auto column = minX; column < maxX; ++column)
            {
                const auto coverage = mask.coverage[size_t(row - y0) * mask.width + size_t(column - x0)];
                auto& alpha = result.rgba[(size_t(row) * result.width + size_t(column)) * 4 + 3];
                // Source-over coverage, not max() or saturating addition. Glyph
                // overlap from kerning/negative spacing retains antialiasing.
                alpha = static_cast<uint8_t>(coverage + (uint32_t(alpha) * (255u - coverage) + 127u) / 255u);
            }
        }
    }
    return result;
}
catch (...)
{
    detail::textWarning("Cannot allocate/rasterize paragraph; no bitmap produced.");
    TextRasterization result;
    result.layout.renderable = false;
    return result;
}

}
