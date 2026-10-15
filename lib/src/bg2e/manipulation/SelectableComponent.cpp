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

#include <bg2e/manipulation/SelectableComponent.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <limits>
#include <stdexcept>

namespace bg2e::manipulation {

SelectableComponent::SelectableComponent()
{
}

std::shared_ptr<scene::Component> SelectableComponent::clone() const
{
    // Pick identifiers are allocated lazily from a global counter in update(), so
    // a clone must start empty to receive its own identifiers instead of copying
    // the source's, which would make two nodes share the same pick ids.
    auto result = std::make_shared<SelectableComponent>();
    result->setPriority(priority());
    result->setIgnoreSerialization(ignoreSerialization());
    return result;
}

void SelectableComponent::update(float)
{
    if (!ownerNode())
    {
        return;
    }

    auto component = ownerNode()->drawable();
    std::shared_ptr<void> drawableIdentity;
    std::shared_ptr<void> meshIdentity;
    uint32_t count = 0;
    if (component && component->executionMode() == scene::DrawableComponent::ExecutionMode::Draw)
    {
        auto drawable = component->drawDrawable();
        drawableIdentity = drawable;
        meshIdentity = drawable->mesh();
        count = drawable->submeshesCount();
    }
    else if (component && component->executionMode() == scene::DrawableComponent::ExecutionMode::Production)
    {
        if (auto drawable = component->drawable())
        {
            drawableIdentity = drawable;
            meshIdentity = drawable->mesh();
            count = drawable->submeshesCount();
        }
    }
    if (_drawableIdentity.lock() != drawableIdentity || _meshIdentity.lock() != meshIdentity || count != _submeshCount)
    {
        _drawableIdentity = drawableIdentity;
        _meshIdentity = meshIdentity;
        _submeshCount = count;
        _identifier.clear();
        _submeshSelected.assign(count, false);
        for (uint32_t i = 0; i < count; ++i)
        {
            _identifier[i] = generateIdentifier();
        }
    }

    // The gizmo identifier is generated regardless of whether the node owns a
    // regular drawable, so light/environment/camera nodes are pickable too.
    if (_gizmoIdentifier == 0)
    {
        _gizmoIdentifier = generateIdentifier();
    }
}

uint32_t SelectableComponent::gizmoIdentifier()
{
    if (_gizmoIdentifier == 0)
    {
        _gizmoIdentifier = generateIdentifier();
    }
    return _gizmoIdentifier;
}

uint32_t SelectableComponent::_lastIdentifier = 0;

uint32_t SelectableComponent::generateIdentifier()
{
    if (_lastIdentifier == std::numeric_limits<uint32_t>::max())
    {
        throw std::overflow_error("Selection identifier capacity exhausted");
    }
    SelectableComponent::_lastIdentifier++;
    return SelectableComponent::_lastIdentifier;
}

void SelectableComponent::deserialize(std::shared_ptr<json::JsonNode> data, const std::filesystem::path&)
{
    deserializeCommon(data);
    // Runtime selection and identifiers never come from a scene document.
    _identifier.clear();
    _submeshSelected.clear();
    _submeshCount = 0;
    _gizmoIdentifier = 0;
    _drawableIdentity.reset();
    _meshIdentity.reset();
}

std::shared_ptr<json::JsonNode> SelectableComponent::serializeReferences(const std::filesystem::path& basePath)
{
    return Component::serialize(basePath);
}

scene::BG2E_SCENE_REGISTER_COMPONENT(SelectableComponent);

}
