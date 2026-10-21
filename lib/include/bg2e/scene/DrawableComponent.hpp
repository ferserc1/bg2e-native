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

#include <optional>
#include <string>

#include <bg2e/scene/Component.hpp>
#include <bg2e/draw/Drawable.hpp>
#include <utility>

namespace bg2e {
namespace scene {

class BG2E_API DrawableComponent : public Component {
  public:
    BG2E_COMPONENT_TYPE_NAME("Drawable");

    DrawableComponent() = default;
    DrawableComponent(std::shared_ptr<bg2e::draw::Drawable> drw) : _drawable{std::move(drw)}
    {
    }
    DrawableComponent(bg2e::draw::Drawable* drw) : _drawable{std::shared_ptr<bg2e::draw::Drawable>(drw)}
    {
    }
    DrawableComponent(std::nullptr_t)
    {
    }
    virtual ~DrawableComponent() = default;

    // Editor overlays may reuse DrawableComponent storage without representing
    // authored scene geometry. Node::drawable excludes those helpers.
    virtual bool isSceneGeometry() const
    {
        return true;
    }

    std::shared_ptr<bg2e::draw::Drawable> drawable();
    std::shared_ptr<const bg2e::draw::Drawable> drawable() const;
    void setDrawable(std::shared_ptr<bg2e::draw::Drawable> drw);
    void setDrawable(std::nullptr_t)
    {
        clearDrawable();
    }
    // Call at a coordinated boundary; other owners retain their realizations.
    void clearDrawable();

    std::shared_ptr<Component> clone() const override;

    // Resolved source reference; setting it does not replace live resources.
    void setResourcePath(const std::filesystem::path& path);
    // Exchange already-normalized references without allocation (transaction rollback).
    void swapResourcePath(std::filesystem::path& path) noexcept
    {
        _resourcePath.swap(path);
    }
    const std::filesystem::path& resourcePath() const
    {
        return _resourcePath;
    }
    void deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath) override;
    std::shared_ptr<json::JsonNode> serializeReferences(const std::filesystem::path& basePath) override;
    void loadResources(bg2e::draw::Engine& engine, SceneLoadProgress* progress = nullptr) override;

    // Reference-only representation. Complete export requires engine-aware DB IO.
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath) override;

  protected:
    std::filesystem::path _resourcePath;
    std::optional<std::string> _resourceName;
    std::shared_ptr<bg2e::draw::Drawable> _drawable;
};

}
}
