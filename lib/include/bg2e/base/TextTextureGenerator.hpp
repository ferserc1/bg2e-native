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

#include <bg2e/base/Texture.hpp>
#include <bg2e/base/TextRasterizer.hpp>

namespace bg2e::base {

// Owns independent editable paragraph data and an immutable resolved font.
// Configure at a stopped resource boundary, not concurrently with generation.
class BG2E_API TextTextureGenerator final : public ProceduralTextureGenerator {
  public:
    TextTextureGenerator(const Text& text, std::shared_ptr<const TextFont> font, uint64_t fontVersion = 0);
    TextTextureGenerator(const TextTextureGenerator& source);
    TextTextureGenerator& operator=(const TextTextureGenerator&) = delete;

    const Text& text() const;
    const std::shared_ptr<const TextFont>& font() const;
    uint64_t fontVersion() const;
    // Conservative CPU ownership accounting for editor resource retention.
    size_t retainedBytes() const noexcept;

    // The caller resolves text.fontIdentifier() externally. No scene or file
    // lookup occurs here. Identical values/font/version keep the pixel revision.
    // A null font is a recoverable unavailable resource: generate transparent.
    bool configure(const Text& text, std::shared_ptr<const TextFont> font, uint64_t fontVersion = 0) noexcept;
    std::shared_ptr<TextTextureGenerator> clone() const noexcept;

    // Repeated requests reuse CPU rasterization but return a fresh new[] buffer
    // for Image's delete[] ownership contract. Failures warn and use transparent
    // output (1x1 under allocation pressure); nullptr means even that failed.
    // Resolution comes from Text; independent dimension edits are recovered.
    uint8_t* generate() noexcept override;
    std::string imageIdentifier() noexcept override;
    base::Color::Type colorType() override;

  private:
    Text _text;
    std::shared_ptr<const TextFont> _font;
    uint64_t _fontVersion = 0;
    uint64_t _identity;
    uint64_t _cachedRevision = 0;
    uint64_t _unavailableRevision = 0;
    std::unique_ptr<TextRasterization> _cache;

    uint8_t* transparentFallback() noexcept;
};

// Initial sampling defaults only. Subsequent configure() calls never overwrite
// user sampling. Ownership/edit/export metadata belongs to scene::TextComponent.
BG2E_API std::shared_ptr<Texture> createTextTexture(std::shared_ptr<TextTextureGenerator> generator) noexcept;

}
