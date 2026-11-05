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

#include <bg2e/base/Text.hpp>
#include <bg2e/base/TextFont.hpp>
#include <bg2e/json/NodeReader.hpp>
#include "TextUtf8.hpp"
#include "TextDiagnostics.hpp"
#include <algorithm>
#include <cmath>

namespace bg2e::base {
namespace {
std::string normalizedContent(const std::string& content)
{
    std::string result;
    result.reserve(std::min(content.size(), Text::MaxContentBytes));
    bool repaired = false;
    for (size_t i = 0; i < content.size();)
    {
        const auto start = i;
        const auto scalar = detail::nextTextScalar(content, i);
        const auto count = scalar == U'\ufffd' ? 3 : i - start;
        if (count > Text::MaxContentBytes - result.size())
        {
            detail::textWarning("Content truncated at a UTF-8 boundary to 64 KiB.");
            break;
        }
        if (scalar == U'\ufffd')
        {
            repaired = repaired || i - start == 1;
            result.append("\xef\xbf\xbd");
        }
        else
        {
            result.append(content, start, i - start);
        }
    }
    if (repaired)
    {
        detail::textWarning("Invalid UTF-8 bytes replaced with U+FFFD.");
    }
    return result;
}


void sanitize(Text::Parameters& p)
{
    bool validIdentifier = !p.fontIdentifier.empty() && p.fontIdentifier.size() <= 128;
    for (unsigned char c : p.fontIdentifier)
    {
        if (!validIdentifier)
        {
            break;
        }
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '_' || c == '-' || c == '.' || c == ':'))
        {
            validIdentifier = false;
        }
    }
    if (!validIdentifier)
    {
        detail::textWarning("Invalid font identifier; using 'default' (no implicit font fallback).");
        p.fontIdentifier = "default";
    }
    const auto scalar = [](float value, float fallback, float minimum, float maximum, const char* field)
    {
        const auto repaired = std::isfinite(value) && !(minimum > 0 && value <= 0) ?
            std::clamp(value, minimum, maximum) : fallback;
        if (!std::isfinite(value) || repaired != value)
        {
            detail::textWarning("Invalid ", field, "; using ", repaired, '.');
        }
        return repaired;
    };
    p.paragraphSize.width = scalar(p.paragraphSize.width, 512, 1, 1000000, "paragraph width");
    p.paragraphSize.height = scalar(p.paragraphSize.height, 256, 1, 1000000, "paragraph height");
    p.fontSize = scalar(p.fontSize, 32, 1, 4096, "font size");
    p.lineSpacing = scalar(p.lineSpacing, 1.2f, 0.1f, 10, "line spacing");
    p.characterSpacing = scalar(p.characterSpacing, 0, -4096, 4096, "character spacing");
    p.margins.left = scalar(p.margins.left, 8, 0, 1000000, "left margin");
    p.margins.top = scalar(p.margins.top, 8, 0, 1000000, "top margin");
    p.margins.right = scalar(p.margins.right, 8, 0, 1000000, "right margin");
    p.margins.bottom = scalar(p.margins.bottom, 8, 0, 1000000, "bottom margin");
    if (double(p.margins.left) + p.margins.right >= p.paragraphSize.width)
    {
        detail::textWarning("Horizontal margins leave no interior; using zero horizontal margins.");
        p.margins.left = p.margins.right = 0;
    }
    if (double(p.margins.top) + p.margins.bottom >= p.paragraphSize.height)
    {
        detail::textWarning("Vertical margins leave no interior; using zero vertical margins.");
        p.margins.top = p.margins.bottom = 0;
    }
    auto& r = p.bitmapResolution;
    if (!r.width || !r.height || r.width > Text::MaxBitmapDimension || r.height > Text::MaxBitmapDimension)
    {
        detail::textWarning("Invalid bitmap dimensions; applying defaults/axis limits.");
        r.width = r.width ? std::min(r.width, Text::MaxBitmapDimension) : 1024;
        r.height = r.height ? std::min(r.height, Text::MaxBitmapDimension) : 512;
    }
    double factor = std::min(1.0, std::sqrt(double(Text::MaxBitmapPixels) / (double(r.width) * r.height)));
    const auto rasterHeight = double(p.fontSize) * std::max(double(r.width) / p.paragraphSize.width,
        double(r.height) / p.paragraphSize.height);
    factor = std::min(factor, double(TextFont::MaxGlyphBitmapDimension) / rasterHeight);
    if (factor < 1)
    {
        detail::textWarning("Bitmap exceeds allocation/glyph-scale limits; reducing resolution without changing layout.");
        r.width = std::max(1u, static_cast<uint32_t>(std::floor(r.width * factor)));
        r.height = std::max(1u, static_cast<uint32_t>(std::floor(r.height * factor)));
    }
    if (p.alignment != Text::Alignment::Left && p.alignment != Text::Alignment::Center && p.alignment != Text::Alignment::Right)
    {
        detail::textWarning("Unknown alignment; using left alignment.");
        p.alignment = Text::Alignment::Left;
    }
    if (p.verticalAlignment != Text::VerticalAlignment::Top && p.verticalAlignment != Text::VerticalAlignment::Center &&
        p.verticalAlignment != Text::VerticalAlignment::Bottom)
    {
        detail::textWarning("Unknown vertical alignment; using top alignment.");
        p.verticalAlignment = Text::VerticalAlignment::Top;
    }
    if (p.wrapMode != Text::WrapMode::None && p.wrapMode != Text::WrapMode::Word && p.wrapMode != Text::WrapMode::Character)
    {
        detail::textWarning("Unknown wrapping; using word wrapping.");
        p.wrapMode = Text::WrapMode::Word;
    }
    if (p.overflowPolicy != Text::OverflowPolicy::Clip && p.overflowPolicy != Text::OverflowPolicy::Hide)
    {
        detail::textWarning("Unknown overflow policy; using clip.");
        p.overflowPolicy = Text::OverflowPolicy::Clip;
    }
}
}


Text::Text(Parameters parameters)
{
    setParameters(std::move(parameters));
}


Text& Text::operator=(const Text& other) noexcept
try
{
    if (this != &other)
    {
        setParameters(other._parameters);
    }
    return *this;
}
catch (...)
{
    warnEditFailure();
    return *this;
}


bool Text::operator==(const Text& other) const
{
    return _parameters == other._parameters;
}


void Text::setParameters(Parameters parameters) noexcept
try
{
    parameters.content = normalizedContent(parameters.content);
    sanitize(parameters);
    if (_parameters != parameters)
    {
        _parameters = std::move(parameters);
        ++_revision;
    }
}
catch (...)
{
    warnEditFailure();
}


void Text::warnEditFailure() noexcept
{
    detail::textWarning("Cannot allocate/update paragraph data; previous values retained.");
}


void Text::setContent(std::string value)
{
    edit([&](auto& p)
    {
        p.content = std::move(value);
    });
}


void Text::setFontIdentifier(std::string value)
{
    edit([&](auto& p)
    {
        p.fontIdentifier = std::move(value);
    });
}


void Text::setFontSize(float value)
{
    edit([&](auto& p)
    {
        p.fontSize = value;
    });
}


void Text::setLineSpacing(float value)
{
    edit([&](auto& p)
    {
        p.lineSpacing = value;
    });
}


void Text::setCharacterSpacing(float value)
{
    edit([&](auto& p)
    {
        p.characterSpacing = value;
    });
}


void Text::setParagraphSize(Size value)
{
    edit([&](auto& p)
    {
        p.paragraphSize = value;
    });
}


void Text::setBitmapResolution(Resolution value)
{
    edit([&](auto& p)
    {
        p.bitmapResolution = value;
    });
}


void Text::setMargins(Margins value)
{
    edit([&](auto& p)
    {
        p.margins = value;
    });
}


void Text::setAlignment(Alignment value)
{
    edit([&](auto& p)
    {
        p.alignment = value;
    });
}


void Text::setVerticalAlignment(VerticalAlignment value)
{
    edit([&](auto& p)
    {
        p.verticalAlignment = value;
    });
}


void Text::setWrapMode(WrapMode value)
{
    edit([&](auto& p)
    {
        p.wrapMode = value;
    });
}


void Text::setOverflowPolicy(OverflowPolicy value)
{
    edit([&](auto& p)
    {
        p.overflowPolicy = value;
    });
}


void Text::deserialize(std::shared_ptr<json::JsonNode> data) noexcept
try
{
    json::ObjectReader reader(data);
    if (!reader.isValid())
    {
        detail::textWarning("Invalid paragraph representation; existing/default values retained.");
        return;
    }
    auto p = _parameters;
    if (auto value = reader.getString("content"))
    {
        p.content = *value;
    }
    if (auto value = reader.getString("fontIdentifier"))
    {
        p.fontIdentifier = *value;
    }
    p.fontSize = reader.getNumber("fontSize").value_or(p.fontSize);
    p.lineSpacing = reader.getNumber("lineSpacing").value_or(p.lineSpacing);
    p.characterSpacing = reader.getNumber("characterSpacing").value_or(p.characterSpacing);
    if (auto size = reader.getObject("paragraphSize"))
    {
        p.paragraphSize.width = size->getNumber("width").value_or(p.paragraphSize.width);
        p.paragraphSize.height = size->getNumber("height").value_or(p.paragraphSize.height);
    }
    if (auto size = reader.getObject("bitmapResolution"))
    {
        const auto dimension = [&](const char* key, uint32_t fallback)
        {
            auto value = size->getNumber(key);
            if (!value)
            {
                return fallback;
            }
            if (*value < 1 || *value > MaxBitmapDimension || std::floor(*value) != *value)
            {
                detail::textWarning("Invalid bitmap dimension in JSON; previous/default value retained.");
                return fallback;
            }
            return static_cast<uint32_t>(*value);
        };
        p.bitmapResolution = {dimension("width", p.bitmapResolution.width), dimension("height", p.bitmapResolution.height)};
    }
    if (auto margins = reader.getObject("margins"))
    {
        p.margins = {margins->getNumber("left").value_or(p.margins.left), margins->getNumber("top").value_or(p.margins.top),
            margins->getNumber("right").value_or(p.margins.right), margins->getNumber("bottom").value_or(p.margins.bottom)};
    }
    if (auto value = reader.getString("alignment"))
    {
        if (*value != "left" && *value != "center" && *value != "right")
        {
            detail::textWarning("Unknown alignment in JSON; using left alignment.");
            *value = "left";
        }
        p.alignment = *value == "left" ? Alignment::Left : *value == "center" ? Alignment::Center : Alignment::Right;
    }
    if (auto value = reader.getString("verticalAlignment"))
    {
        if (*value != "top" && *value != "center" && *value != "bottom")
        {
            detail::textWarning("Unknown vertical alignment in JSON; using top alignment.");
            *value = "top";
        }
        p.verticalAlignment = *value == "top" ? VerticalAlignment::Top :
            *value == "center" ? VerticalAlignment::Center : VerticalAlignment::Bottom;
    }
    if (auto value = reader.getString("wrapMode"))
    {
        if (*value != "none" && *value != "word" && *value != "character")
        {
            detail::textWarning("Unknown wrap mode in JSON; using word wrapping.");
            *value = "word";
        }
        p.wrapMode = *value == "none" ? WrapMode::None : *value == "word" ? WrapMode::Word : WrapMode::Character;
    }
    if (auto value = reader.getString("overflowPolicy"))
    {
        if (*value != "clip" && *value != "hide" && *value != "error")
        {
            detail::textWarning("Unknown overflow policy in JSON; using clip.");
            *value = "clip";
        }
        p.overflowPolicy = *value == "clip" ? OverflowPolicy::Clip : OverflowPolicy::Hide;
    }
    setParameters(std::move(p));
}
catch (...)
{
    detail::textWarning("Cannot deserialize paragraph; previous/default values retained.");
}


std::shared_ptr<json::JsonNode> Text::serialize() noexcept
try
{
    using namespace json;
    const auto& p = _parameters;
    return JSON(JsonObject{
        {"content", JSON(p.content)}, {"fontIdentifier", JSON(p.fontIdentifier)},
        {"fontSize", JSON(p.fontSize)}, {"lineSpacing", JSON(p.lineSpacing)}, {"characterSpacing", JSON(p.characterSpacing)},
        {"paragraphSize", JSON(JsonObject{{"width", JSON(p.paragraphSize.width)}, {"height", JSON(p.paragraphSize.height)}})},
        {"bitmapResolution", JSON(JsonObject{{"width", JSON(p.bitmapResolution.width)}, {"height", JSON(p.bitmapResolution.height)}})},
        {"margins", JSON(JsonObject{{"left", JSON(p.margins.left)}, {"top", JSON(p.margins.top)},
            {"right", JSON(p.margins.right)}, {"bottom", JSON(p.margins.bottom)}})},
        {"alignment", JSON(p.alignment == Alignment::Left ? "left" : p.alignment == Alignment::Center ? "center" : "right")},
        {"verticalAlignment", JSON(p.verticalAlignment == VerticalAlignment::Top ? "top" :
            p.verticalAlignment == VerticalAlignment::Center ? "center" : "bottom")},
        {"wrapMode", JSON(p.wrapMode == WrapMode::None ? "none" : p.wrapMode == WrapMode::Word ? "word" : "character")},
        {"overflowPolicy", JSON(p.overflowPolicy == OverflowPolicy::Clip ? "clip" : "hide")}});
}
catch (...)
{
    detail::textWarning("Cannot serialize paragraph; no representation produced.");
    return nullptr;
}

}
