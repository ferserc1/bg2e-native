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

#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/utils/utils.hpp>
#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/db/mesh_obj.hpp>
#include <algorithm>
#include <bg2e/gpu/Device.hpp>

#include <iostream>
#include <stdexcept>

namespace bg2e::scene {

std::shared_ptr<bg2e::draw::Drawable> DrawableComponent::drawable()
{
    return _drawable;
}

std::shared_ptr<const bg2e::draw::Drawable> DrawableComponent::drawable() const
{
    return _drawable;
}

void DrawableComponent::setDrawable(std::shared_ptr<bg2e::draw::Drawable> drw)
{
    if (_drawable && drw && _drawable->engine() && drw->engine() &&
        _drawable->engine() != drw->engine())
    {
        throw std::logic_error("Clear DrawableComponent before selecting a different draw context");
    }
    if (_drawable && _drawable->engine())
    {
        _drawable->engine()->device()->waitIdle();
    }
    _drawable = std::move(drw);
}

void DrawableComponent::clearDrawable()
{
    if (_drawable && _drawable->engine())
    {
        _drawable->engine()->device()->waitIdle();
    }
    _drawable.reset();
}

std::shared_ptr<Component> DrawableComponent::clone() const
{
    auto copy = std::make_shared<DrawableComponent>();

    // Deep copy the drawable so the duplicated component owns an independent mesh,
    // materials and GPU resources. This is the whole point of cloning by copy: the
    // two DrawableComponents must never share the same Drawable instance.

    if (_drawable)
    {
        copy->_drawable = _drawable->clone();
    }

    copy->_resourcePath = _resourcePath;
    copy->_resourceName = _resourceName;
    copy->_priority = _priority;
    copy->_ignoreSerialization = _ignoreSerialization;
    return copy;
}

void DrawableComponent::setResourcePath(const std::filesystem::path& path)
{
    _resourcePath = path.empty() ? path : std::filesystem::absolute(path).lexically_normal();
}

void DrawableComponent::deserialize(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath)
{
    if (_drawable)
    {
        throw std::logic_error("Clear DrawableComponent before reading a resource reference");
    }
    deserializeCommon(data);
    json::ObjectReader reader(data);
    _resourceName = reader.getString("resourceName");
    if (auto path = reader.getString("path"))
    {
        if (path->empty())
        {
            throw std::invalid_argument("Drawable source path cannot be empty");
        }
        setResourcePath(basePath / *path);
    }
    else if (auto name = reader.getString("name"))
    {
        if (name->empty())
        {
            throw std::invalid_argument("Drawable source name cannot be empty");
        }
        auto path = basePath / *name;
        path.replace_extension(".bg2");
        setResourcePath(path);
    }
    else
    {
        throw std::invalid_argument("Drawable representation requires a path or legacy name");
    }
    if (_resourcePath.empty())
    {
        throw std::invalid_argument("Drawable source path cannot be empty");
    }
}

std::shared_ptr<json::JsonNode> DrawableComponent::serializeReferences(const std::filesystem::path& basePath)
{
    if (_resourcePath.empty())
    {
        throw std::logic_error("Drawable needs an explicit source path for reference-only serialization");
    }
    auto result = Component::serialize(basePath);
    const auto base = basePath.empty() ? std::filesystem::current_path() : std::filesystem::absolute(basePath);
    const auto relative = _resourcePath.lexically_relative(base.lexically_normal());
    if (_drawable)
    {
        result->objectValue()["resourceName"] = json::JSON(_drawable->name());
    }
    else if (_resourceName)
    {
        result->objectValue()["resourceName"] = json::JSON(*_resourceName);
    }
    result->objectValue()["path"] = json::JSON((relative.empty() ? _resourcePath : relative).generic_string());
    return result;
}

void DrawableComponent::loadResources(bg2e::draw::Engine& engine, SceneLoadProgress* progress)
{
    if (_drawable)
    {
        if (!_drawable->isLoaded())
        {
            _drawable->load(&engine);
        }
        else if (_drawable->engine() != &engine)
        {
            throw std::logic_error("DrawableComponent belongs to another draw engine");
        }
        return;
    }
    auto onTextureLoaded = [progress]()
    {
        if (progress)
        {
            progress->total = std::max(progress->total, progress->loaded + 1);
            if (progress->callback)
            {
                progress->callback("texture", progress->loaded, progress->total);
            }
            ++progress->loaded;
        }
    };
    if (_resourcePath.extension() == ".bg2" || _resourcePath.extension() == ".vwglb")
    {
        setDrawable(db::loadDrawableBg2(_resourcePath, engine, onTextureLoaded));
    }
    else if (_resourcePath.extension() == ".obj")
    {
        setDrawable(db::loadDrawableObj(_resourcePath, engine, onTextureLoaded));
    }
    else
    {
        throw std::invalid_argument("Unsupported draw resource: " + _resourcePath.string());
    }
    if (_resourceName)
    {
        _drawable->setName(*_resourceName);
    }
}

std::shared_ptr<json::JsonNode> DrawableComponent::serialize(const std::filesystem::path& basePath)
{
    return serializeReferences(basePath);
}

BG2E_SCENE_REGISTER_COMPONENT(DrawableComponent);

}
