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

#include "ComponentCommands.hpp"

#include <bg2e/reflection/Registry.hpp>
#include <bg2e/scene/PolarTransformController.hpp>
#include <bg2e/scene/FixedScaleTransformController.hpp>
#include <bg2e/manipulation/GizmoComponent.hpp>
#include <algorithm>
#include <stdexcept>
#include <typeinfo>
#include <cmath>

namespace bg2e_composer::command::component {
using namespace bg2e::app;
using Component = bg2e::scene::Component;
namespace {
const bg2e::reflection::PropertyInfo& resolve(Component& component, const bg2e::ui::ReflectionEditIntent& intent, void*& owner)
{
    const auto* type = bg2e::reflection::TypeRegistry::get().type(component.typeName());
    const auto* property = type ? bg2e::ui::ReflectionWidget::resolveProperty(&component, *type, intent.path, owner) : nullptr;
    if (!property || !property->getter || !property->setter || property->type != intent.type)
    {
        throw std::invalid_argument("Reflected property path or storage type changed");
    }
    return *property;
}

bool projectionEdit(const bg2e::ui::ReflectionEditIntent& intent)
{
    return intent.type == bg2e::reflection::PropertyType::PolymorphicObject &&
        intent.path.size() == 1 && intent.path.front().property == "projection";
}

bool equalScalar(const std::any& a, const std::any& b)
{
    if (a.type() != b.type())
    {
        return false;
    }
    if (!a.has_value())
    {
        return true;
    }
    if (a.type() == typeid(bool))
    {
        return std::any_cast<const bool&>(a) == std::any_cast<const bool&>(b);
    }
    if (a.type() == typeid(int32_t))
    {
        return std::any_cast<const int32_t&>(a) == std::any_cast<const int32_t&>(b);
    }
    if (a.type() == typeid(uint32_t))
    {
        return std::any_cast<const uint32_t&>(a) == std::any_cast<const uint32_t&>(b);
    }
    if (a.type() == typeid(int64_t))
    {
        return std::any_cast<const int64_t&>(a) == std::any_cast<const int64_t&>(b);
    }
    if (a.type() == typeid(float))
    {
        return std::any_cast<const float&>(a) == std::any_cast<const float&>(b);
    }
    if (a.type() == typeid(double))
    {
        return std::any_cast<const double&>(a) == std::any_cast<const double&>(b);
    }
    if (a.type() == typeid(std::string))
    {
        return std::any_cast<const std::string&>(a) == std::any_cast<const std::string&>(b);
    }
    if (a.type() == typeid(glm::vec2))
    {
        return std::any_cast<const glm::vec2&>(a) == std::any_cast<const glm::vec2&>(b);
    }
    if (a.type() == typeid(glm::vec3))
    {
        return std::any_cast<const glm::vec3&>(a) == std::any_cast<const glm::vec3&>(b);
    }
    if (a.type() == typeid(glm::vec4))
    {
        return std::any_cast<const glm::vec4&>(a) == std::any_cast<const glm::vec4&>(b);
    }
    if (a.type() == typeid(glm::mat4))
    {
        return std::any_cast<const glm::mat4&>(a) == std::any_cast<const glm::mat4&>(b);
    }
    if (const auto* value = std::any_cast<bg2e::base::Color>(&a))
    {
        const auto& other = std::any_cast<const bg2e::base::Color&>(b);
        return value->r == other.r && value->g == other.g && value->b == other.b && value->a == other.a;
    }
    return false;
}

bool finiteValue(const std::any& value)
{
    if (const auto* number = std::any_cast<float>(&value))
    {
        return std::isfinite(*number);
    }
    if (const auto* number = std::any_cast<double>(&value))
    {
        return std::isfinite(*number);
    }
    if (const auto* vector = std::any_cast<glm::vec2>(&value))
    {
        return std::isfinite(vector->x) && std::isfinite(vector->y);
    }
    if (const auto* vector = std::any_cast<glm::vec3>(&value))
    {
        return std::isfinite(vector->x) && std::isfinite(vector->y) && std::isfinite(vector->z);
    }
    if (const auto* vector = std::any_cast<glm::vec4>(&value))
    {
        return std::isfinite(vector->x) && std::isfinite(vector->y) && std::isfinite(vector->z) && std::isfinite(vector->w);
    }
    return true;
}

void refreshTools(bg2e::scene::Node& node)
{
    if (auto gizmo = node.getComponent<bg2e::manipulation::GizmoComponent>())
    {
        gizmo->synchronizeResources();
    }
}
}

EditMembership::EditMembership(std::shared_ptr<StageScene> stage,
    std::shared_ptr<bg2e::manipulation::SelectionManager> selection,
    std::weak_ptr<bg2e::scene::Node> node, std::shared_ptr<Component> original, std::shared_ptr<Component> replacement)
    : _stage(stage), _selection(selection), _node(std::move(node))
{
    if (auto owner = _node.lock())
    {
        for (const auto& component : owner->orderedComponents())
        {
            if (!supportedComponent(*component))
            {
                throw std::invalid_argument("Custom component membership requires a restoration adapter");
            }
        }
    }
    if (original)
    {
        _original = stage->retainForCommand(original);
    }
    if (replacement)
    {
        _replacement = stage->retainForCommand(replacement);
    }
}

const CommandInfo& EditMembership::info() const noexcept
{
    return _info;
}

CommandMemory EditMembership::memory() const
{
    CommandMemory result{sizeof(*this) + 16384, {}};
    for (const auto& lease : {_original, _replacement})
    {
        if (lease)
        {
            result.resources.push_back({lease, lease->bytes});
            result.localBytes += lease->growthBytes();
        }
    }
    if (auto stage = _stage.lock())
    {
        result.localBytes += 4 * stage->editingStateBytes();
    }
    if (auto node = _node.lock())
    {
        result.localBytes += 4 * (node->components().size() + 1) * sizeof(std::weak_ptr<Component>);
    }
    if (auto selection = _selection.lock())
    {
        result.localBytes += 4 * (selection->selectedNodes().size() + selection->selectedItems().size() + 1) *
            (sizeof(bg2e::manipulation::SelectionItem) + sizeof(std::weak_ptr<bg2e::scene::Node>));
    }
    return result;
}

void EditMembership::publish(const std::vector<std::weak_ptr<Component>>& value)
{
    auto node = _node.lock();
    std::vector<std::shared_ptr<Component>> components;
    for (const auto& weak : value)
    {
        auto component = weak.lock();
        if (!component)
        {
            throw std::logic_error("Component identity expired");
        }
        components.push_back(std::move(component));
    }
    node->setComponentsOrdered(components);
    refreshTools(*node);
}

CommandResult EditMembership::execute() noexcept
{
    auto stage = _stage.lock();
    auto node = _node.lock();
    auto selection = _selection.lock();
    bool publishing = false;
    try
    {
        if (!stage || !node || !selection || (!stage->contains(node.get()) && node != stage->fillLightNode()))
        {
            return {CommandStatus::Rejected, "Component owner is stale"};
        }
        auto old = _original ? _original->component : nullptr;
        auto next = _replacement ? _replacement->component : nullptr;
        if ((!old && !next) || old == next)
        {
            return {};
        }
        if ((old && old->ownerNode() != node.get()) || (next && next->ownerNode()))
        {
            return {CommandStatus::Rejected, "Component ownership changed"};
        }
        const auto* oldInstance = old.get();
        const auto* nextInstance = next.get();
        if (old && next && typeid(*oldInstance) != typeid(*nextInstance))
        {
            return {CommandStatus::Rejected, "Replacement must retain the concrete component type"};
        }
        _before.clear();
        for (const auto& component : node->orderedComponents())
        {
            _before.push_back(component);
            if (component != old && next && component->typeName() == next->typeName())
            {
                return {CommandStatus::Rejected, "The component type is already attached"};
            }
        }
        _after = _before;
        if (old)
        {
            auto position = std::find_if(_after.begin(), _after.end(), [&](const auto& weak)
            {
                return weak.lock() == old;
            });
            if (next)
            {
                next->setPriority(old->priority());
                next->setIgnoreSerialization(old->ignoreSerialization());
                *position = next;
            }
            else
            {
                _after.erase(position);
            }
        }
        else
        {
            _after.push_back(next);
        }
        _beforeState = stage->editingState();
        _beforeSelection = selection->captureSelection();
        publishing = true; // A throwing attach/detach hook may have published membership.
        publish(_after);
        stage->validateEditingGraph(false);
        stage->refreshCamera(true);
        _afterState = stage->editingState();
        selection->restoreSelection(_beforeSelection, [stage](auto* item)
        {
            return stage->contains(item) || item == stage->fillLightNode().get();
        });
        _afterSelection = selection->captureSelection();
        _captured = true;
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        if (publishing)
        {
            try
            {
                publish(_before);
                stage->restoreEditingState(_beforeState);
                selection->restoreSelection(_beforeSelection, [stage](auto* item)
                {
                    return stage->contains(item) || item == stage->fillLightNode().get();
                });
            }
            catch (...)
            {
                return {CommandStatus::Faulted, "Component membership recovery failed"};
            }
        }
        return {CommandStatus::Failed, "Component membership edit rejected"};
    }
}

CommandResult EditMembership::restore(bool forward) noexcept
{
    auto stage = _stage.lock();
    auto node = _node.lock();
    auto selection = _selection.lock();
    if (!_captured || !stage || !node || !selection || (!stage->contains(node.get()) && node != stage->fillLightNode()))
    {
        return {CommandStatus::Rejected, "Component snapshot is stale"};
    }
    try
    {
        publish(forward ? _after : _before);
        stage->validateEditingGraph(false);
        stage->restoreEditingState(forward ? _afterState : _beforeState);
        selection->restoreSelection(forward ? _afterSelection : _beforeSelection,
            [stage](auto* item)
            {
                return stage->contains(item) || item == stage->fillLightNode().get();
            });
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        try
        {
            publish(forward ? _before : _after);
            stage->restoreEditingState(forward ? _beforeState : _afterState);
            selection->restoreSelection(forward ? _beforeSelection : _afterSelection,
                [stage](auto* item)
                {
                    return stage->contains(item) || item == stage->fillLightNode().get();
                });
        }
        catch (...)
        {
            return {CommandStatus::Faulted, "Component membership replay recovery failed"};
        }
        return {CommandStatus::Failed, "Could not restore component membership"};
    }
}

CommandResult EditMembership::undo() noexcept
{
    return restore(false);
}

CommandResult EditMembership::redo() noexcept
{
    return restore(true);
}

bool supportedProperty(Component& component, const bg2e::ui::ReflectionEditIntent& intent)
{
    using namespace bg2e::scene;
    using bg2e::reflection::PropertyType;
    if (!supportedComponent(component) || dynamic_cast<DrawableComponent*>(&component) ||
        dynamic_cast<EnvironmentComponent*>(&component) || intent.path.empty() || !finiteValue(intent.proposed))
    {
        return false; // Resource/custom effects need their dedicated adapter.
    }
    if (auto* camera = dynamic_cast<CameraComponent*>(&component))
    {
        const auto* projection = camera->projection();
        if (!projection || (typeid(*projection) != typeid(bg2e::math::PerspectiveProjection) &&
            typeid(*projection) != typeid(bg2e::math::OpticalProjection) &&
            typeid(*projection) != typeid(bg2e::math::OrthographicProjection)))
        {
            return false;
        }
        if (projectionEdit(intent))
        {
            const auto* key = std::any_cast<std::string>(&intent.proposed);
            return key && (*key == "PerspectiveProjection" || *key == "OpticalProjection" || *key == "OrthographicProjection");
        }
    }
    if (intent.type == PropertyType::Object || intent.type == PropertyType::PolymorphicObject ||
        intent.type == PropertyType::Resource || intent.type == PropertyType::Path)
    {
        return false;
    }
    if (auto* transform = dynamic_cast<TransformComponent*>(&component))
    {
        auto* node = transform->ownerNode();
        if (node)
        {
            for (const auto& entry : node->components())
            {
                if (!supportedComponent(*entry.second))
                {
                    return false;
                }
            }
        }
        // A live writer needs an authored transform adapter; never race its
        // evaluated pose against a buffered inspector edit.
        if (!node || node->getComponent<OrbitCameraComponent>() || node->getComponent<PolarTransformControllerComponent>() ||
            node->getComponent<FixedScaleTransformControllerComponent>())
        {
            return false;
        }
    }
    try
    {
        void* owner = nullptr;
        resolve(component, intent, owner);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

EditProperty::EditProperty(std::weak_ptr<StageScene> stage, std::weak_ptr<Component> component,
    bg2e::ui::ReflectionEditIntent intent, Action action)
    : _stage(std::move(stage)), _target(std::move(component)), _intent(std::move(intent)), _action(action)
{
}

const CommandInfo& EditProperty::info() const noexcept
{
    return _info;
}

CommandMemory EditProperty::memory() const
{
    size_t bytes = sizeof(*this) + 65536;
    for (const auto& segment : _intent.path)
    {
        bytes += segment.ownerType.capacity() + segment.property.capacity() + segment.subtypeKey.capacity();
    }
    if (const auto* text = std::any_cast<std::string>(&_intent.proposed); text && text->size() > 4096)
    {
        throw std::length_error("Property draft exceeds the supported scalar budget");
    }
    if (auto stage = _stage.lock())
    {
        bytes += 4 * stage->editingStateBytes();
    }
    return {bytes, {}};
}

EditProperty::Value EditProperty::capture(Component& component) const
{
    using namespace bg2e::scene;
    Value value;
    if (auto* transform = dynamic_cast<TransformComponent*>(&component))
    {
        value.matrix = transform->matrix();
    }
    if (_action == Action::ResetProjection || projectionEdit(_intent))
    {
        value.projection = static_cast<CameraComponent&>(component).camera().projectionResource();
    }
    else if (_action == Action::ResetOrbit)
    {
        auto& orbit = static_cast<OrbitCameraComponent&>(component);
        value.pose = Pose{orbit.rotation(), orbit.center(), orbit.distance()};
    }
    else if (_action == Action::Property)
    {
        void* owner = nullptr;
        const auto& property = resolve(component, _intent, owner);
        value.scalar = property.getter(owner);
    }
    return value;
}

void EditProperty::write(Component& component, const Value& value) const
{
    using namespace bg2e::scene;
    if (value.matrix)
    {
        static_cast<TransformComponent&>(component).setMatrix(*value.matrix);
    }
    else if (value.projection)
    {
        static_cast<CameraComponent&>(component).setProjection(value.projection);
    }
    else if (value.pose)
    {
        auto& orbit = static_cast<OrbitCameraComponent&>(component);
        orbit.setRotation(value.pose->rotation);
        orbit.setCenter(value.pose->center);
        orbit.setDistance(value.pose->distance);
    }
    else
    {
        void* owner = nullptr;
        resolve(component, _intent, owner).setter(owner, value.scalar);
    }
}

void EditProperty::propose(Component& component) const
{
    using namespace bg2e::scene;
    if (_action == Action::TransformIdentity)
    {
        static_cast<TransformComponent&>(component).setIdentity();
    }
    else if (_action == Action::ResetOrbit)
    {
        static_cast<OrbitCameraComponent&>(component).reset();
    }
    else if (_action == Action::ResetProjection || projectionEdit(_intent))
    {
        const auto key = _action == Action::ResetProjection ? "PerspectiveProjection" : std::any_cast<std::string>(_intent.proposed);
        std::shared_ptr<bg2e::math::Projection> replacement;
        if (key == "PerspectiveProjection")
        {
            replacement = std::make_shared<bg2e::math::PerspectiveProjection>();
        }
        else if (key == "OpticalProjection")
        {
            replacement = std::make_shared<bg2e::math::OpticalProjection>();
        }
        else if (key == "OrthographicProjection")
        {
            replacement = std::make_shared<bg2e::math::OrthographicProjection>();
        }
        else
        {
            throw std::invalid_argument("Projection subtype requires a dedicated adapter");
        }
        auto& camera = static_cast<CameraComponent&>(component);
        replacement->setNear(camera.projection()->near());
        replacement->setFar(camera.projection()->far());
        replacement->setViewport(camera.projection()->viewport());
        camera.setProjection(std::move(replacement));
    }
    else
    {
        void* owner = nullptr;
        resolve(component, _intent, owner).setter(owner, _intent.proposed);
    }
}

CommandResult EditProperty::execute() noexcept
{
    auto stage = _stage.lock();
    auto target = _target.lock();
    if (!stage || !target || (!stage->contains(target->ownerNode()) && target->ownerNode() != stage->fillLightNode().get()) || !supportedComponent(*target) ||
        (_action == Action::Property && !supportedProperty(*target, _intent)))
    {
        return {CommandStatus::Rejected, "This property/action requires a reversible editor adapter"};
    }
    if ((_action == Action::TransformIdentity && !dynamic_cast<bg2e::scene::TransformComponent*>(target.get())) ||
        (_action == Action::ResetProjection && !dynamic_cast<bg2e::scene::CameraComponent*>(target.get())) ||
        (_action == Action::ResetOrbit && !dynamic_cast<bg2e::scene::OrbitCameraComponent*>(target.get())))
    {
        return {CommandStatus::Rejected, "Action adapter does not match the component type"};
    }
    if (_action == Action::TransformIdentity)
    {
        auto* node = target->ownerNode();
        if (node->getComponent<bg2e::scene::OrbitCameraComponent>() ||
            node->getComponent<bg2e::scene::PolarTransformControllerComponent>() ||
            node->getComponent<bg2e::scene::FixedScaleTransformControllerComponent>())
        {
            return {CommandStatus::Rejected, "A controller owns this evaluated transform"};
        }
    }
    if (_action == Action::ResetProjection)
    {
        bg2e::ui::ReflectionEditIntent request;
        request.path.push_back({target->typeName(), "projection", {}, false});
        request.type = bg2e::reflection::PropertyType::PolymorphicObject;
        request.proposed = std::string("PerspectiveProjection");
        if (!supportedProperty(*target, request))
        {
            return {CommandStatus::Rejected, "Projection reset requires a supported projection instance"};
        }
    }
    bool writing = false;
    try
    {
        _beforeState = stage->editingState();
        _before = capture(*target);
        writing = true;
        propose(*target);
        stage->validateEditingGraph(false);
        stage->refreshCamera(true);
        _after = capture(*target);
        _afterState = stage->editingState();
        bool same = _before.matrix ? _after.matrix && *_before.matrix == *_after.matrix :
            _before.pose ? _after.pose && _before.pose->rotation == _after.pose->rotation &&
                _before.pose->center == _after.pose->center && _before.pose->distance == _after.pose->distance :
            _before.projection ? _before.projection == _after.projection : equalScalar(_before.scalar, _after.scalar);
        if (same)
        {
            write(*target, _before);
            stage->restoreEditingState(_beforeState);
            return {CommandStatus::NoOp, {}};
        }
        _captured = true;
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        if (writing)
        {
            try
            {
                write(*target, _before);
                stage->restoreEditingState(_beforeState);
            }
            catch (...)
            {
                return {CommandStatus::Faulted, "Property recovery failed"};
            }
        }
        return {CommandStatus::Failed, "Property violates the editor contract or scene constraints"};
    }
}

CommandResult EditProperty::restore(bool forward) noexcept
{
    auto stage = _stage.lock();
    auto target = _target.lock();
    if (!_captured || !stage || !target || (!stage->contains(target->ownerNode()) && target->ownerNode() != stage->fillLightNode().get()))
    {
        return {CommandStatus::Rejected, "Property target is stale"};
    }
    Value recovery;
    StageScene::EditingState recoveryState;
    bool writing = false;
    try
    {
        recovery = capture(*target);
        recoveryState = stage->editingState();
        writing = true;
        write(*target, forward ? _after : _before);
        stage->validateEditingGraph(false);
        stage->restoreEditingState(forward ? _afterState : _beforeState);
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        if (writing)
        {
            try
            {
                write(*target, recovery);
                stage->restoreEditingState(recoveryState);
            }
            catch (...)
            {
                return {CommandStatus::Faulted, "Property replay recovery failed"};
            }
        }
        return {CommandStatus::Failed, "Could not restore property"};
    }
}

CommandResult EditProperty::undo() noexcept
{
    return restore(false);
}

CommandResult EditProperty::redo() noexcept
{
    return restore(true);
}

}
