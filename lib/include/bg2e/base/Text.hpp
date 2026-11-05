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
#include <bg2e/json/JsonNode.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace bg2e::base {

// CPU paragraph data only. Logical units determine layout; bitmap dimensions
// only determine sampling density. World-space quad dimensions belong to the
// future component, not to this data or the rasterizer. No appearance is stored.
class BG2E_API Text {
  public:
    enum class Alignment
    {
        Left, Center, Right
    };
    enum class VerticalAlignment
    {
        Top, Center, Bottom
    };
    enum class WrapMode
    {
        None, Word, Character
    };
    enum class OverflowPolicy
    {
        Clip, Hide,
        Error = Hide // Source compatibility: hide with a warning, NEVER throw.
    };

    struct Size {
        float width = 512, height = 256;
        bool operator==(const Size&) const = default;
    };
    struct Resolution {
        uint32_t width = 1024, height = 512;
        bool operator==(const Resolution&) const = default;
    };
    struct Margins {
        float left = 8, top = 8, right = 8, bottom = 8;
        bool operator==(const Margins&) const = default;
    };
    struct Parameters {
        std::string content;
        std::string fontIdentifier = "default"; // Explicit identifier, not a fallback.
        float fontSize = 32; // Logical ascent-to-descent height.
        float lineSpacing = 1.2f; // Multiplier of the font's native line advance.
        float characterSpacing = 0; // Additional logical advance between glyphs.
        Size paragraphSize;
        Resolution bitmapResolution;
        Margins margins;
        Alignment alignment = Alignment::Left;
        VerticalAlignment verticalAlignment = VerticalAlignment::Top;
        WrapMode wrapMode = WrapMode::Word;
        OverflowPolicy overflowPolicy = OverflowPolicy::Clip;
        bool operator==(const Parameters&) const = default;
    };
    static constexpr size_t MaxContentBytes = 64 * 1024;
    static constexpr uint32_t MaxBitmapDimension = 8192;
    static constexpr size_t MaxBitmapPixels = 16 * 1024 * 1024;

    Text() = default;
    explicit Text(Parameters parameters);
    Text(const Text&) = default;
    Text& operator=(const Text& other) noexcept;
    bool operator==(const Text& other) const;

    const Parameters& parameters() const
    {
        return _parameters;
    }
    // Recover invalid fields with defaults/limits and warn, never throw for text
    // data errors. Failed allocation leaves the previously usable state intact.
    void setParameters(Parameters parameters) noexcept;
    uint64_t revision() const
    {
        return _revision;
    }

    const std::string& content() const
    {
        return _parameters.content;
    }
    // Invalid UTF-8 bytes become U+FFFD (one replacement per invalid byte).
    void setContent(std::string value);
    const std::string& fontIdentifier() const
    {
        return _parameters.fontIdentifier;
    }
    void setFontIdentifier(std::string value);
    float fontSize() const
    {
        return _parameters.fontSize;
    }
    void setFontSize(float value);
    float lineSpacing() const
    {
        return _parameters.lineSpacing;
    }
    void setLineSpacing(float value);
    float characterSpacing() const
    {
        return _parameters.characterSpacing;
    }
    void setCharacterSpacing(float value);

    Size paragraphSize() const
    {
        return _parameters.paragraphSize;
    }
    void setParagraphSize(Size value);
    Resolution bitmapResolution() const
    {
        return _parameters.bitmapResolution;
    }
    void setBitmapResolution(Resolution value);
    Margins margins() const
    {
        return _parameters.margins;
    }
    void setMargins(Margins value);
    Alignment alignment() const
    {
        return _parameters.alignment;
    }
    void setAlignment(Alignment value);
    VerticalAlignment verticalAlignment() const
    {
        return _parameters.verticalAlignment;
    }
    void setVerticalAlignment(VerticalAlignment value);
    WrapMode wrapMode() const
    {
        return _parameters.wrapMode;
    }
    void setWrapMode(WrapMode value);
    OverflowPolicy overflowPolicy() const
    {
        return _parameters.overflowPolicy;
    }
    void setOverflowPolicy(OverflowPolicy value);

    void deserialize(std::shared_ptr<json::JsonNode> data) noexcept;
    std::shared_ptr<json::JsonNode> serialize() noexcept;

  private:
    Parameters _parameters;
    uint64_t _revision = 1;

    static void warnEditFailure() noexcept;

    template<class Edit>
    void edit(Edit operation)
    {
        try
        {
            auto next = _parameters;
            operation(next);
            setParameters(std::move(next));
        }
        catch (...)
        {
            warnEditFailure();
        }
    }
};

}
