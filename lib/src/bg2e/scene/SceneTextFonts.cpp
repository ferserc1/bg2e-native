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

#include <bg2e/scene/Scene.hpp>
#include <bg2e/scene/TextFontComponent.hpp>
#include "../base/TextDiagnostics.hpp"
#include <functional>
#include <set>
#include <utility>

namespace bg2e::scene {

void Scene::invalidateTextFonts() noexcept
{
    _textFontsDirty = true;
    _textFontWarnings.clear();
    ++_textFontVersion;
}


bool Scene::textFontAvailable(const std::string& identifier) const noexcept
{
    const auto found = _textFonts.find(identifier);
    return !_textFontsDirty && found != _textFonts.end() && found->second.font != nullptr;
}


bool Scene::textFontsReady() const
{
    return !_textFontsDirty;
}


uint64_t Scene::textFontVersion() const
{
    return _textFontVersion;
}


std::shared_ptr<const base::TextFont> Scene::textFont(const std::string& identifier) const noexcept
try
{
    if (identifier.empty() || identifier.size() > 128)
    {
        base::detail::textWarning("Invalid scene font lookup identifier; font unavailable.");
        return nullptr;
    }
    const auto found = _textFonts.find(identifier);
    if (_textFontsDirty || found == _textFonts.end() || !found->second.font)
    {
        if (_textFontWarnings.size() < 128 && _textFontWarnings.insert(identifier).second)
        {
            base::detail::textWarning("Scene font '", identifier,
                "' is missing, ambiguous, unavailable or not prepared; dependent text must be transparent.");
        }
        return nullptr;
    }
    return found->second.font;
}
catch (...)
{
    base::detail::textWarning("Cannot resolve scene font; font unavailable.");
    return nullptr;
}


void Scene::reloadTextFonts() noexcept
{
    invalidateTextFonts();
    _reloadTextFonts = true;
    prepareTextFonts();
}


void Scene::prepareTextFonts() noexcept
{
    if (!_textFontsDirty)
    {
        return;
    }
    try
    {
        std::map<std::string, FontResource> next;
        std::map<std::string, std::string> owners;
        std::set<std::string> ambiguous;
        bool capacityWarning = false;
        std::function<void(Node*)> collect = [&](Node* node)
        {
            if (!node)
            {
                return;
            }
            // Declarations on disabled nodes still belong to the scene namespace.
            for (const auto& component : node->orderedComponents())
            {
                const auto* fonts = dynamic_cast<TextFontComponent*>(component.get());
                if (!fonts)
                {
                    continue;
                }
                for (const auto& declaration : fonts->declarations())
                {
                    if (!TextFontComponent::validateDeclaration(declaration))
                    {
                        continue;
                    }
                    if (next.size() >= TextFontComponent::MaxDeclarations && !next.contains(declaration.identifier))
                    {
                        if (!capacityWarning)
                        {
                            base::detail::textWarning("Scene font capacity reached; additional declarations ignored.");
                            capacityWarning = true;
                        }
                        continue;
                    }
                    auto [owner, inserted] = owners.emplace(declaration.identifier, node->name());
                    if (!inserted)
                    {
                        next.find(declaration.identifier)->second.ambiguous = true;
                        if (ambiguous.insert(declaration.identifier).second)
                        {
                            base::detail::textWarning("Duplicate scene font '", declaration.identifier,
                                "' on nodes '", owner->second, "' and '", node->name(), "'; identifier disabled.");
                        }
                        continue;
                    }
                    next.emplace(declaration.identifier, FontResource{declaration.path, declaration.faceIndex, nullptr});
                }
            }
            for (const auto& child : node->children())
            {
                collect(child.get());
            }
        };
        collect(_sceneRoot.get()); // Validate every identifier before the first file load.
        size_t bytes = 0;
        constexpr size_t MaxRegistryBytes = 128 * 1024 * 1024;
        for (auto& [identifier, resource] : next)
        {
            if (resource.ambiguous)
            {
                continue;
            }
            const auto previous = _textFonts.find(identifier);
            if (!_reloadTextFonts && previous != _textFonts.end() && !previous->second.ambiguous &&
                !previous->second.retryOnChange &&
                previous->second.path == resource.path && previous->second.faceIndex == resource.faceIndex)
            {
                resource.font = previous->second.font;
            }
            else
            {
                resource.font = base::TextFont::load(resource.path, resource.faceIndex);
            }
            if (!resource.font)
            {
                continue; // Failed sources stay unavailable until edit/explicit reload.
            }
            if (resource.font->byteSize() > MaxRegistryBytes - bytes)
            {
                base::detail::textWarning("Scene font byte budget exceeded; font '", identifier, "' disabled.");
                resource.font.reset();
                resource.retryOnChange = true;
                continue;
            }
            bytes += resource.font->byteSize();
        }
        _textFonts.swap(next); // No partially loaded registry becomes visible.
        _textFontsDirty = false;
        _reloadTextFonts = false;
        _textFontWarnings.clear();
        ++_textFontVersion;
    }
    catch (...)
    {
        _textFonts.clear();
        _textFontsDirty = false; // Do not retry an allocation/I/O failure per frame.
        _reloadTextFonts = false;
        ++_textFontVersion;
        base::detail::textWarning("Cannot prepare scene font registry; fonts unavailable until edit/explicit reload.");
    }
}

}
