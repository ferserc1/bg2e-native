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

#include <bg2e/scene/TextFontComponent.hpp>
#include <bg2e/base/TextFont.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/json/NodeReader.hpp>
#include "../base/TextDiagnostics.hpp"
#include <algorithm>
#include <set>
#include <optional>
#include <utility>

namespace bg2e::scene {

const std::vector<TextFontComponent::Declaration>& TextFontComponent::declarations() const
{
    return _declarations;
}


bool TextFontComponent::validateDeclaration(const Declaration& declaration) noexcept
try
{
    const auto& id = declaration.identifier;
    const auto path = declaration.path.generic_string();
    if (id.empty() || id.size() > 128 || declaration.path.empty() ||
        path.size() > 16384 || path.find('\0') != std::string::npos ||
        declaration.faceIndex >= base::TextFont::MaxFaces)
    {
        base::detail::textWarning("Invalid font declaration (identifier/path/face); declaration skipped.");
        return false;
    }
    // Stable portable identifiers are intentionally independent of filenames.
    for (unsigned char c : id)
    {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '_' || c == '-' || c == '.' || c == ':'))
        {
            base::detail::textWarning("Invalid font identifier '", id, "'; declaration skipped.");
            return false;
        }
    }
    return true;
}
catch (...)
{
    base::detail::textWarning("Cannot inspect font declaration; declaration skipped.");
    return false;
}


void TextFontComponent::setDeclarations(std::vector<Declaration> declarations) noexcept
try
{
    if (declarations.size() > MaxDeclarations)
    {
        base::detail::textWarning("Font collection exceeds 128 entries; extra declarations ignored.");
        declarations.resize(MaxDeclarations);
    }
    std::set<std::string> identifiers;
    std::vector<Declaration> valid;
    for (auto declaration : declarations)
    {
        if (declaration.faceIndex >= base::TextFont::MaxFaces)
        {
            base::detail::textWarning("Invalid font face index; using face 0.");
            declaration.faceIndex = 0;
        }
        if (!validateDeclaration(declaration))
        {
            continue;
        }
        // Lexical normalization does not access the filesystem. File I/O is
        // deferred until the scene's explicit font preparation boundary.
        std::error_code error;
        auto path = std::filesystem::absolute(declaration.path, error);
        if (error)
        {
            base::detail::textWarning("Cannot resolve font path; declaration skipped: ", declaration.identifier);
            continue;
        }
        declaration.path = path.lexically_normal();
        if (declaration.notices.size() > 32)
        {
            base::detail::textWarning("Too many font notices; extra entries ignored.");
            declaration.notices.resize(32);
        }
        for (auto& notice : declaration.notices)
        {
            notice = std::filesystem::absolute(notice).lexically_normal();
        }
        if (!identifiers.insert(declaration.identifier).second)
        {
            base::detail::textWarning("Duplicate font identifier in collection: ", declaration.identifier,
                "; later declaration ignored.");
            continue;
        }
        valid.push_back(std::move(declaration));
    }
    if (_declarations != valid)
    {
        _declarations = std::move(valid);
        if (auto* owner = scene())
        {
            owner->invalidateTextFonts();
        }
    }
}
catch (...)
{
    base::detail::textWarning("Cannot update font declarations; previous declarations retained.");
}


void TextFontComponent::addFont(std::string identifier, std::filesystem::path path, uint32_t faceIndex)
try
{
    auto next = _declarations;
    next.push_back({std::move(identifier), std::move(path), faceIndex, {}});
    setDeclarations(std::move(next));
}
catch (...)
{
    base::detail::textWarning("Cannot add font declaration; previous declarations retained.");
}


void TextFontComponent::removeFont(const std::string& identifier)
try
{
    auto next = _declarations;
    std::erase_if(next, [&](const auto& declaration)
    {
        return declaration.identifier == identifier;
    });
    setDeclarations(std::move(next));
}
catch (...)
{
    base::detail::textWarning("Cannot remove font declaration; previous declarations retained.");
}


std::shared_ptr<Component> TextFontComponent::clone() const noexcept
try
{
    auto copy = std::make_shared<TextFontComponent>();
    copy->_declarations = _declarations;
    copy->setPriority(priority());
    copy->setIgnoreSerialization(ignoreSerialization());
    return copy;
}
catch (...)
{
    base::detail::textWarning("Cannot clone font declarations; clone unavailable.");
    return nullptr;
}


void TextFontComponent::deserialize(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath) noexcept
try
{
    json::ObjectReader reader(data);
    auto fonts = reader.getArray("fonts");
    if (!reader.isValid() || !fonts)
    {
        base::detail::textWarning("Missing/invalid fonts array; previous/default declarations retained.");
        return;
    }
    if (fonts->size() > MaxDeclarations)
    {
        base::detail::textWarning("Too many font declarations in JSON; extra entries ignored.");
    }
    std::vector<Declaration> declarations;
    for (size_t i = 0; i < std::min(fonts->size(), MaxDeclarations); ++i)
    {
        auto font = fonts->getObject(i);
        auto id = font ? font->getString("identifier") : std::nullopt;
        auto path = font ? font->getString("path") : std::nullopt;
        if (!font || !id || !path || path->empty())
        {
            base::detail::textWarning("Malformed font declaration skipped.");
            continue;
        }
        uint32_t face = 0;
        if (!font->isUndefined("faceIndex"))
        {
            auto value = font->getInteger<uint32_t>("faceIndex");
            if (!value)
            {
                base::detail::textWarning("Invalid font faceIndex in JSON; using face 0.");
            }
            face = value.value_or(0);
        }
        auto resolved = std::filesystem::path(*path);
        if (resolved.is_relative())
        {
            resolved = basePath / resolved;
        }
        Declaration declaration{*id, std::move(resolved), face, {}};
        if (auto notices = font->getArray("notices"))
        {
            for (size_t n = 0; n < std::min(notices->size(), size_t(32)); ++n)
            {
                const auto& value = notices->node()->listValue()[n];
                if (value && value->isString() && !value->stringValue().empty())
                {
                    declaration.notices.push_back(basePath / value->stringValue());
                }
                else
                {
                    base::detail::textWarning("Invalid font notice path ignored.");
                }
            }
        }
        declarations.push_back(std::move(declaration));
    }
    setDeclarations(std::move(declarations));
    deserializeCommon(data);
}
catch (...)
{
    base::detail::textWarning("Cannot deserialize font declarations; existing/default state retained.");
}


std::shared_ptr<json::JsonNode> TextFontComponent::serializeReferences(const std::filesystem::path& basePath) noexcept
try
{
    auto result = Component::serialize(basePath);
    json::JsonList fonts;
    const auto directory = (basePath.empty() ? std::filesystem::current_path() :
        std::filesystem::absolute(basePath)).lexically_normal();
    for (const auto& declaration : _declarations)
    {
        auto relative = declaration.path.lexically_relative(directory);
        const auto path = relative.empty() ? declaration.path : relative;
        json::JsonList notices;
        for (const auto& notice : declaration.notices)
        {
            auto relativeNotice = notice.lexically_relative(directory);
            notices.push_back(json::JSON((relativeNotice.empty() ? notice : relativeNotice).generic_string()));
        }
        fonts.push_back(json::JSON(json::JsonObject{
            {"identifier", json::JSON(declaration.identifier)},
            {"path", json::JSON(path.generic_string())},
            {"faceIndex", json::JSON(declaration.faceIndex)},
            {"notices", json::JSON(notices)}}));
    }
    result->objectValue()["fonts"] = json::JSON(fonts);
    return result;
}
catch (...)
{
    base::detail::textWarning("Cannot serialize font declarations; component representation omitted.");
    return nullptr;
}


std::shared_ptr<json::JsonNode> TextFontComponent::serialize(const std::filesystem::path& basePath) noexcept
{
    return serializeReferences(basePath);
}


void TextFontComponent::loadResources(draw::Engine&, SceneLoadProgress*) noexcept
{
    if (auto* owner = scene())
    {
        owner->prepareTextFonts();
    }
    else if (!_declarations.empty())
    {
        base::detail::textWarning("Detached font declarations have no scene registry; font preparation deferred.");
    }
}


void TextFontComponent::didAddToNode(Node* owner)
{
    if (auto* scene = owner->scene())
    {
        scene->invalidateTextFonts();
    }
}


void TextFontComponent::didRemoveFromNode(Node* owner)
{
    if (auto* scene = owner->scene())
    {
        scene->invalidateTextFonts();
    }
}


BG2E_SCENE_REGISTER_COMPONENT(TextFontComponent);

}
