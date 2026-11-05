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

#include <bg2e/base/TextTextureGenerator.hpp>
#include "TextDiagnostics.hpp"
#include <algorithm>
#include <atomic>
#include <limits>
#include <new>
#include <utility>

namespace bg2e::base {

size_t TextTextureGenerator::retainedBytes() const noexcept
{
    size_t bytes = sizeof(*this) + _text.content().capacity() + _text.fontIdentifier().capacity();
    if (_font)
    {
        bytes += _font->byteSize();
    }
    if (_cache)
    {
        bytes += sizeof(*_cache) + _cache->rgba.capacity() +
            _cache->layout.glyphs.capacity() * sizeof(TextLayout::Glyph) +
            _cache->layout.lines.capacity() * sizeof(TextLayout::Line);
    }
    return bytes;
}

namespace {
uint64_t nextGeneratorIdentity()
{
    // Process-local identity allocator, not a font/resource registry. Distinct
    // paragraphs/clones cannot alias even when their local revisions coincide.
    static std::atomic<uint64_t> next{1};
    auto value = next.load(std::memory_order_relaxed);
    for (;;)
    {
        if (value == std::numeric_limits<uint64_t>::max())
        {
            detail::textWarning("Generator identity space exhausted; using instance-local identity.");
            return 0;
        }
        if (next.compare_exchange_weak(value, value + 1, std::memory_order_relaxed))
        {
            return value;
        }
    }
}
}


TextTextureGenerator::TextTextureGenerator(const Text& text, std::shared_ptr<const TextFont> font, uint64_t fontVersion)
    : ProceduralTextureGenerator(text.bitmapResolution().width, text.bitmapResolution().height, 4),
      _font(std::move(font)), _fontVersion(fontVersion), _identity(nextGeneratorIdentity())
{
    _text = text;
    const auto resolution = _text.bitmapResolution();
    setDimensions(resolution.width, resolution.height, 4);
}


TextTextureGenerator::TextTextureGenerator(const TextTextureGenerator& source)
    : TextTextureGenerator(source._text, source._font, source._fontVersion)
{
}


const Text& TextTextureGenerator::text() const
{
    return _text;
}


const std::shared_ptr<const TextFont>& TextTextureGenerator::font() const
{
    return _font;
}


uint64_t TextTextureGenerator::fontVersion() const
{
    return _fontVersion;
}


bool TextTextureGenerator::configure(const Text& text, std::shared_ptr<const TextFont> font, uint64_t fontVersion) noexcept
{
    if (_text == text && _font == font && _fontVersion == fontVersion)
    {
        return false;
    }
    _text = text; // Validate/copy before publishing the nonthrowing resource state.
    if (!(_text == text))
    {
        return false; // Allocation failure retained the previous paragraph.
    }
    _font = std::move(font);
    _fontVersion = fontVersion;
    const auto resolution = _text.bitmapResolution();
    setDimensions(resolution.width, resolution.height, 4);
    markModified();
    _cache.reset();
    return true;
}


std::shared_ptr<TextTextureGenerator> TextTextureGenerator::clone() const noexcept
try
{
    return std::make_shared<TextTextureGenerator>(*this);
}
catch (...)
{
    detail::textWarning("Cannot clone text generator; clone unavailable.");
    return nullptr;
}


uint8_t* TextTextureGenerator::generate() noexcept
try
{
    if (_unavailableRevision == revision())
    {
        return transparentFallback();
    }
    const auto resolution = _text.bitmapResolution();
    if (width() != resolution.width || height() != resolution.height || bytesPerPixel() != 4)
    {
        detail::textWarning("Independent generator dimension edit ignored; restoring authored text dimensions.");
        setDimensions(resolution.width, resolution.height, 4);
    }
    if (!_cache || _cachedRevision != revision())
    {
        auto candidate = std::make_unique<TextRasterization>();
        if (_font)
        {
            *candidate = TextRasterizer::rasterize(_text, *_font);
        }
        else
        {
            detail::textWarning("Font '", _text.fontIdentifier(), "' is unavailable; generating transparent text.");
            candidate->width = resolution.width;
            candidate->height = resolution.height;
            candidate->layout.renderable = false;
            candidate->rgba.assign(size_t(resolution.width) * resolution.height * 4, 255);
            for (size_t alpha = 3; alpha < candidate->rgba.size(); alpha += 4)
            {
                candidate->rgba[alpha] = 0;
            }
        }
        if (candidate->rgba.empty())
        {
            return transparentFallback();
        }
        _cache = std::move(candidate);
        _cachedRevision = revision();
    }
    auto* result = new (std::nothrow) uint8_t[_cache->rgba.size()];
    if (!result)
    {
        return transparentFallback();
    }
    std::copy(_cache->rgba.begin(), _cache->rgba.end(), result);
    return result;
}
catch (...)
{
    return transparentFallback();
}


uint8_t* TextTextureGenerator::transparentFallback() noexcept
{
    if (_unavailableRevision != revision())
    {
        detail::textWarning("Paragraph bitmap unavailable; using transparent 1x1 fallback until its source changes.");
    }
    _cache.reset();
    setDimensions(1, 1, 4);
    _unavailableRevision = revision();
    // nullptr is the last-resort signal if even four bytes cannot be allocated.
    // Callers must omit an unavailable image, never dereference/upload it.
    return new (std::nothrow) uint8_t[4]{255, 255, 255, 0};
}


std::string TextTextureGenerator::imageIdentifier() noexcept
try
{
    // Every pixel-producing value is owned by Text and changes revision through
    // configure(). An immutable font replacement or version change does likewise.
    // This deliberately is not a cross-process/disk-cache content hash.
    return "text-paragraph:" + std::to_string(_identity) + ":instance:" +
        std::to_string(reinterpret_cast<uintptr_t>(this)) + ":" + std::to_string(revision()) +
        ":font-version:" + std::to_string(_fontVersion);
}
catch (...)
{
    detail::textWarning("Cannot allocate paragraph image identifier; identifier unavailable.");
    return {};
}


base::Color::Type TextTextureGenerator::colorType()
{
    return base::Color::TypeLinear;
}


std::shared_ptr<Texture> createTextTexture(std::shared_ptr<TextTextureGenerator> generator) noexcept
try
{
    if (!generator)
    {
        detail::textWarning("Cannot create text texture without a generator; texture unavailable.");
        return nullptr;
    }
    auto result = std::make_shared<Texture>(std::move(generator));
    result->setMagFilter(Texture::FilterLinear);
    result->setMinFilter(Texture::FilterLinear);
    result->setAddressMode(Texture::AddressModeClampToEdge, Texture::AddressModeClampToEdge, Texture::AddressModeClampToEdge);
    result->setUseMipmaps(false);
    result->setColorType(Color::TypeLinear);
    return result;
}
catch (...)
{
    detail::textWarning("Cannot create text texture; texture unavailable.");
    return nullptr;
}

}
