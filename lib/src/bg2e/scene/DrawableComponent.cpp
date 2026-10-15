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
#include <bg2e/gpu/Device.hpp>

#include <iostream>
#include <stdexcept>

namespace bg2e::scene {

DrawableComponent::ExecutionMode DrawableComponent::executionMode() const
{
    return _drawDrawable ? ExecutionMode::Draw : _drawable ? ExecutionMode::Production : ExecutionMode::Uninitialized;
}
std::shared_ptr<bg2e::draw::Drawable> DrawableComponent::drawDrawable()
{
    if (_drawable)
    {
        throw std::logic_error("DrawableComponent contains a production drawable");
    }
    return _drawDrawable;
}
std::shared_ptr<const bg2e::draw::Drawable> DrawableComponent::drawDrawable() const
{
    if (_drawable)
    {
        throw std::logic_error("DrawableComponent contains a production drawable");
    }
    return _drawDrawable;
}
void DrawableComponent::setDrawable(std::shared_ptr<bg2e::draw::Drawable> drw)
{
    if (_drawable)
    {
        throw std::logic_error("Clear DrawableComponent before selecting draw execution");
    }
    if (_drawDrawable && drw && _drawDrawable->engine() && drw->engine() &&
        _drawDrawable->engine() != drw->engine())
    {
        throw std::logic_error("Clear DrawableComponent before selecting a different draw context");
    }
    if (_drawDrawable && _drawDrawable->engine())
    {
        _drawDrawable->engine()->device()->waitIdle();
    }
    _drawDrawable = std::move(drw);
}
void DrawableComponent::clearDrawable()
{
    // Production callers retain their existing safe-update synchronization rule.
    if (_drawDrawable && _drawDrawable->engine())
    {
        _drawDrawable->engine()->device()->waitIdle();
    }
    _drawDrawable.reset();
    _drawable.reset();
}

std::shared_ptr<DrawableBase> DrawableComponent::drawableBase()
{
    if (_drawDrawable)
    {
        throw std::logic_error("DrawableComponent contains a draw drawable");
    }
    return _drawable;
}

const std::shared_ptr<DrawableBase> DrawableComponent::drawableBase() const
{
    if (_drawDrawable)
    {
        throw std::logic_error("DrawableComponent contains a draw drawable");
    }
    return _drawable;
}

std::shared_ptr<Drawable> DrawableComponent::drawable()
{
    return std::dynamic_pointer_cast<Drawable>(drawableBase());
}

const std::shared_ptr<Drawable> DrawableComponent::drawable() const
{
    return std::dynamic_pointer_cast<Drawable>(drawableBase());
}

void DrawableComponent::setDrawable(std::shared_ptr<DrawableBase> drw)
{
    if (_drawDrawable)
    {
        throw std::logic_error("Clear DrawableComponent before selecting production execution");
    }
    _drawable = std::move(drw);
}

std::shared_ptr<Component> DrawableComponent::clone() const
{
    auto copy = std::make_shared<DrawableComponent>();

    // Deep copy the drawable so the duplicated component owns an independent mesh,
    // materials and GPU resources. This is the whole point of cloning by copy: the
    // two DrawableComponents must never share the same DrawableBase instance.
    if (_drawable)
    {
        copy->_drawable = _drawable->clone();
    }

    if (_drawDrawable)
    {
        copy->_drawDrawable = _drawDrawable->clone();
    }

    copy->_resourcePath = _resourcePath;
    copy->_priority = _priority;
    copy->_ignoreSerialization = _ignoreSerialization;
    return copy;
}

void DrawableComponent::draw(
    const glm::mat4& nodeTransform,
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    std::function<std::vector<VkDescriptorSet>(render::MaterialBase*, const glm::mat4&, uint32_t submIndex)> cb,
    VkPipelineBindPoint bp)
{
    auto drawable = drawableBase().get();
    if (drawable)
    {
        drawable->setTransform(nodeTransform);
        drawable->draw(cmd, layout, cb, bp);
    }
}

void DrawableComponent::setResourcePath(const std::filesystem::path& path)
{
    _resourcePath = path.empty() ? path : std::filesystem::absolute(path).lexically_normal();
}

void DrawableComponent::deserialize(std::shared_ptr<json::JsonNode> data, const std::filesystem::path& basePath)
{
    if (_drawable || _drawDrawable)
    {
        throw std::logic_error("Clear DrawableComponent before reading a resource reference");
    }
    deserializeCommon(data);
    json::ObjectReader reader(data);
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
    result->objectValue()["path"] = json::JSON((relative.empty() ? _resourcePath : relative).generic_string());
    return result;
}

void DrawableComponent::loadResources(bg2e::draw::Engine& engine, SceneLoadProgress* progress)
{
    if (_drawable)
    {
        throw std::logic_error("Cannot load draw resources into a production DrawableComponent");
    }
    if (_drawDrawable)
    {
        if (!_drawDrawable->isLoaded())
        {
            _drawDrawable->load(&engine);
        }
        else if (_drawDrawable->engine() != &engine)
        {
            throw std::logic_error("DrawableComponent belongs to another draw engine");
        }
        return;
    }
    auto onTextureLoaded = [progress]()
    {
        if (progress)
        {
            if (progress->callback)
            {
                progress->callback("texture", progress->loaded, progress->total);
            }
            ++progress->loaded;
        }
    };
    if (_resourcePath.extension() == ".bg2")
    {
        setDrawable(db::loadDrawableBg2(_resourcePath, engine, onTextureLoaded));
    }
    else if (_resourcePath.extension() == ".obj")
    {
        setDrawable(db::loadDrawableObj(_resourcePath, engine));
    }
    else
    {
        throw std::invalid_argument("Unsupported draw resource: " + _resourcePath.string());
    }
}

void DrawableComponent::deserialize(
    std::shared_ptr<json::JsonNode> data,
    const std::filesystem::path& basePath,
    render::Engine& engine)
{
    deserializeWithProgress(data, basePath, engine, nullptr);
}

void DrawableComponent::deserializeWithProgress(
    std::shared_ptr<json::JsonNode> data,
    const std::filesystem::path& basePath,
    render::Engine& engine,
    SceneLoadProgress* progress)
{
    if (_drawDrawable)
    {
        throw std::logic_error("Cannot deserialize production data into a draw DrawableComponent");
    }
    // Resolve through the same CPU parser, preserving the existing live object
    // if reading or realizing its replacement fails.
    DrawableComponent reference;
    reference.deserialize(data, basePath);
    auto onTextureLoaded = [progress]()
    {
        if (progress)
        {
            if (progress->callback)
            {
                progress->callback("texture", progress->loaded, progress->total);
            }
            ++progress->loaded;
        }
    };
    std::shared_ptr<DrawableBase> result;
    if (reference.resourcePath().extension() == ".bg2")
    {
        result = db::loadDrawableBg2(reference.resourcePath(), &engine, onTextureLoaded);
    }
    else if (reference.resourcePath().extension() == ".obj")
    {
        result = db::loadDrawableObj(reference.resourcePath(), &engine);
    }
    else
    {
        throw std::invalid_argument("Unsupported production drawable resource: " + reference.resourcePath().string());
    }
    deserializeCommon(data);
    _resourcePath = reference.resourcePath();
    _drawable = std::move(result);
}

std::shared_ptr<json::JsonNode> DrawableComponent::serialize(const std::filesystem::path& basePath)
{
    if (_drawDrawable)
    {
        throw std::logic_error("Draw resource export is not implemented; use saveSceneReferences for references only");
    }
    if (!_drawable && !_resourcePath.empty())
    {
        throw std::logic_error("Unresolved drawable requires saveSceneReferences or explicit resource loading");
    }
    using namespace bg2e::json;
    auto compData = Component::serialize(basePath);
    JsonObject& obj = compData->objectValue();

    Drawable* drawable = dynamic_cast<Drawable*>(_drawable.get());
    if (drawable)
    {
        if (_drawable->name() == "")
        {
            _drawable->setName(utils::uniqueId());
        }

        obj["name"] = JSON(_drawable->name());

        auto filePath = basePath;
        filePath.append(_drawable->name());
        filePath.replace_extension(".bg2");

        db::storeDrawableBg2(filePath, this);
    }

    return compData;
}

BG2E_SCENE_REGISTER_COMPONENT(DrawableComponent);

}
