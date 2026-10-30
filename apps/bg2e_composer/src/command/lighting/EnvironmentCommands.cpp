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

#include "EnvironmentCommands.hpp"

#include <bg2e/gpu/Image.hpp>
#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <typeinfo>
#include <set>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace bg2e_composer::command::lighting {
namespace {
bool equal(const bg2e::scene::EnvironmentComponent& a, const bg2e::scene::EnvironmentComponent& b)
{
    const auto color = [](const auto& x, const auto& y)
    {
        return x.r == y.r && x.g == y.g && x.b == y.b && x.a == y.a;
    };
    return a.environmentImage() == b.environmentImage() && a.skyboxVisible() == b.skyboxVisible() &&
        a.skyboxBlur() == b.skyboxBlur() && color(a.skyHorizonColor(), b.skyHorizonColor()) &&
        color(a.skyZenithColor(), b.skyZenithColor()) && color(a.groundColor(), b.groundColor());
}

size_t checked(size_t a, size_t b, bool product = false)
{
    if (product ? (a && b > std::numeric_limits<size_t>::max() / a) : b > std::numeric_limits<size_t>::max() - a)
    {
        throw std::length_error("Environment history memory overflow");
    }
    return product ? a * b : a + b;
}

void account(bg2e::app::CommandMemory& memory,
    const std::shared_ptr<const bg2e::draw::environment::EnvironmentResources>& resources)
{
    if (!resources)
    {
        throw std::invalid_argument("Environment history requires a published GPU state");
    }
    memory.resources.push_back({resources, 65536});
    for (const auto& image : {resources->sky, resources->irradiance, resources->prefilteredSpecular, resources->brdf})
    {
        if (!image)
        {
            throw std::invalid_argument("Incomplete retained environment");
        }
        auto width = image->width(), height = image->height();
        const auto texel = bg2e::gpu::pixelFormatBytes(image->pixelFormat());
        if (!texel)
        {
            throw std::invalid_argument("Unsupported retained environment format");
        }
        size_t bytes = 0;
        for (uint32_t level = 0; level < image->mipLevels(); ++level)
        {
            bytes = checked(bytes, checked(checked(width, height, true), texel, true));
            width = std::max(1u, width / 2);
            height = std::max(1u, height / 2);
        }
        memory.resources.push_back({image, checked(bytes, image->arrayLayers(), true)});
    }
}
}

EditEnvironment::EditEnvironment(std::weak_ptr<StageScene> stage,
    std::shared_ptr<bg2e::scene::EnvironmentComponent> target,
    std::shared_ptr<bg2e::scene::EnvironmentComponent> prepared, std::function<Generator*()> generator)
    : _stage(std::move(stage)), _target(target), _generator(std::move(generator))
{
    _identity = _generator();
    if (!target || !prepared || !_identity)
    {
        throw std::invalid_argument("Environment editing requires an active environment and generator");
    }
    if (!supportedComponent(*target) || !supportedComponent(*prepared))
    {
        throw std::invalid_argument("Custom environments require an explicit state adapter");
    }
    _before = std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(target->clone());
    _after = std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(prepared->clone());
    _alternate = std::move(prepared);
    _expected = _identity->resources();
    _beforeResources = _identity->captureState();
    auto owner = _stage.lock();
    auto active = owner ? owner->environmentNode() : nullptr;
    _afterResources = active && active->environment() == target.get()
        ? _identity->prepareState(_after.get()) : _identity->captureState();
    _memory.localBytes = checked(65536, checked(_before->environmentImage().capacity(),
        checked(_after->environmentImage().capacity(), _alternate->environmentImage().capacity())));
    account(_memory, _expected);
    account(_memory, _afterResources.resources);
}

const bg2e::app::CommandInfo& EditEnvironment::info() const noexcept
{
    return _info;
}

bg2e::app::CommandMemory EditEnvironment::memory() const
{
    return _memory;
}

bg2e::app::CommandResult EditEnvironment::execute() noexcept
{
    try
    {
        if (_generator() != _identity || _identity->resources() != _expected)
        {
            return {bg2e::app::CommandStatus::Rejected, "Prepared environment publication changed"};
        }
        return equal(*_before, *_after) ? bg2e::app::CommandResult{} : exchange(true);
    }
    catch (...)
    {
        return {bg2e::app::CommandStatus::Rejected, "Prepared environment validation failed"};
    }
}

bg2e::app::CommandResult EditEnvironment::undo() noexcept
{
    return exchange(false);
}

bg2e::app::CommandResult EditEnvironment::redo() noexcept
{
    return exchange(true);
}

bg2e::app::CommandResult EditEnvironment::exchange(bool applied) noexcept
{
    using namespace bg2e::app;
    if (_applied == applied)
    {
        return {};
    }
    try
    {
        auto stage = _stage.lock();
        auto target = _target.lock();
        auto* generator = _generator();
        if (!stage || !target || !target->ownerNode() || !stage->contains(target->ownerNode()) ||
            !generator || !generator->acceptsState(applied ? _afterResources : _beforeResources) ||
            !equal(*target, _applied ? *_after : *_before))
        {
            return {CommandStatus::Rejected, "Prepared environment target or renderer changed"};
        }
        auto resources = applied ? _afterResources : _beforeResources;
        target->exchangeValues(*_alternate);
        generator->exchangeState(resources);
        _applied = applied;
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        return {CommandStatus::Rejected, "Environment target validation failed"};
    }
}


AdoptEnvironmentPreset::AdoptEnvironmentPreset(const std::shared_ptr<StageScene>& stage,
    const std::shared_ptr<Node>& preset, std::function<Generator*()> generator)
    : _stage(stage), _root(stage->document().root), _generator(std::move(generator))
{
    stage->validateEnvironmentPreset(preset);
    _identity = _generator();
    if (!_identity)
    {
        throw std::invalid_argument("Environment preset requires a generation service");
    }
    auto root = _root.lock();
    auto rig = std::make_shared<Node>("Environment rig");
    rig->addComponent(preset->environment()->clone());
    rig->addComponent(new bg2e::scene::TransformComponent(glm::inverse(root->worldMatrix())));
    auto floor = stage->floorNode();
    auto fill = stage->fillLightNode();
    _before.editing = stage->editingState();
    _before.floor = floor->transform()->matrix();
    _before.floorEnabled = floor->enabled();
    _before.fill = fill->transform()->matrix();
    for (const auto& child : root->children())
    {
        _before.children.push_back(child);
    }
    _after = _before;
    std::set<std::string> fixtures;
    for (const auto& child : preset->children())
    {
        if ((child->name() == "Stage floor" || child->name() == "Stage fill light") &&
            !fixtures.insert(child->name()).second)
        {
            throw std::invalid_argument("Environment preset repeats a stage fixture");
        }
        if (child->name() == "Stage floor")
        {
            _after.floor = child->transform()->matrix();
            _after.floorEnabled = child->enabled();
        }
        else if (child->name() == "Stage fill light")
        {
            Membership membership;
            membership.node = fill;
            for (const auto& component : fill->orderedComponents())
            {
                membership.before.push_back(component);
                if (dynamic_cast<bg2e::scene::LightComponent*>(component.get()))
                {
                    if (!supportedComponent(*component))
                    {
                        throw std::invalid_argument("Custom fill lights require a preset adapter");
                    }
                    _retained.push_back(component);
                }
                else
                {
                    membership.after.push_back(component);
                }
            }
            if (child->light())
            {
                auto light = child->light()->clone();
                membership.after.push_back(light);
                _retained.push_back(std::move(light));
                _after.fill = child->transform()->matrix();
            }
            _after.editing.fillEnabled = child->enabled();
            _memberships.push_back(std::move(membership));
        }
        else if (child->light())
        {
            rig->addChild(child->clone());
        }
    }
    const auto capture = [&](const auto& self, const std::shared_ptr<Node>& node) -> void
    {
        if (node->environment() || node->light())
        {
            Membership membership;
            membership.node = node;
            for (const auto& component : node->orderedComponents())
            {
                membership.before.push_back(component);
                if (dynamic_cast<bg2e::scene::EnvironmentComponent*>(component.get()) ||
                    dynamic_cast<bg2e::scene::LightComponent*>(component.get()))
                {
                    if (!supportedComponent(*component))
                    {
                        throw std::invalid_argument("Custom environment/light components require a preset adapter");
                    }
                    _retained.push_back(component);
                }
                else
                {
                    membership.after.push_back(component);
                }
            }
            _memberships.push_back(std::move(membership));
        }
        for (const auto& child : node->children())
        {
            self(self, child);
        }
    };
    capture(capture, root);
    try
    {
        stage->prepareTools(rig);
        _rig = stage->retainPreparedEnvironment(rig);
    }
    catch (...)
    {
        stage->discardCandidate(rig);
        throw;
    }
    for (auto& membership : _memberships)
    {
        std::stable_sort(membership.after.begin(), membership.after.end(), [](const auto& a, const auto& b)
        {
            return a.lock()->priority() > b.lock()->priority();
        });
    }
    _after.children.push_back(rig);
    _after.editing.explicitEnvironment = true;
    auto environment = std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(rig->environment()->shared_from_this());
    _after.editing.environment = environment;
    _after.editing.knownEnvironments.clear();
    _after.editing.knownEnvironments.emplace(rig->identifier(), environment);
    _expected = _identity->resources();
    _beforeResources = _identity->captureState();
    _afterResources = _identity->prepareState(environment.get());
    _memory.localBytes = checked(65536, checked(_memberships.capacity() * (sizeof(Membership) + 1024), stage->editingStateBytes() * 8));
    for (const auto& membership : _memberships)
    {
        _memory.localBytes = checked(_memory.localBytes,
            checked(membership.before.capacity(), membership.after.capacity()) * sizeof(std::weak_ptr<Component>) * 8);
    }
    for (const auto& component : _retained)
    {
        _memory.localBytes = checked(_memory.localBytes, 65536);
        if (auto value = std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(component))
        {
            _memory.localBytes = checked(_memory.localBytes, value->environmentImage().capacity());
        }
    }
    _memory.localBytes = checked(_memory.localBytes, (_before.children.capacity() + _after.children.capacity()) * sizeof(std::weak_ptr<Node>) * 8);
    _memory.resources.push_back({_rig, _rig->bytes});
    account(_memory, _expected);
    account(_memory, _afterResources.resources);
}

const bg2e::app::CommandInfo& AdoptEnvironmentPreset::info() const noexcept
{
    return _info;
}

bg2e::app::CommandMemory AdoptEnvironmentPreset::memory() const
{
    return _memory;
}

bg2e::app::CommandResult AdoptEnvironmentPreset::execute() noexcept
{
    try
    {
        if (_generator() != _identity || _identity->resources() != _expected)
        {
            return {bg2e::app::CommandStatus::Rejected, "Prepared preset publication changed"};
        }
        return exchange(true);
    }
    catch (...)
    {
        return {bg2e::app::CommandStatus::Rejected, "Prepared preset validation failed"};
    }
}

bg2e::app::CommandResult AdoptEnvironmentPreset::undo() noexcept
{
    return exchange(false);
}

bg2e::app::CommandResult AdoptEnvironmentPreset::redo() noexcept
{
    return exchange(true);
}

void AdoptEnvironmentPreset::assign(bool after)
{
    auto stage = _stage.lock();
    auto root = _root.lock();
    const auto& state = after ? _after : _before;
    std::vector<std::shared_ptr<Node>> children;
    for (const auto& weak : state.children)
    {
        auto child = weak.lock();
        if (!child)
        {
            throw std::logic_error("Preset child identity is stale");
        }
        children.push_back(std::move(child));
    }
    std::vector<std::vector<std::shared_ptr<Component>>> lists;
    std::vector<std::shared_ptr<Node>> owners;
    for (const auto& membership : _memberships)
    {
        auto node = membership.node.lock();
        if (!node)
        {
            throw std::logic_error("Preset component owner is stale");
        }
        owners.push_back(std::move(node));
        auto& list = lists.emplace_back();
        for (const auto& weak : after ? membership.after : membership.before)
        {
            auto component = weak.lock();
            if (!component)
            {
                throw std::logic_error("Preset component identity is stale");
            }
            list.push_back(std::move(component));
        }
    }
    Node::setChildLists({{root, std::move(children)}});
    for (size_t i = 0; i < owners.size(); ++i)
    {
        owners[i]->setComponentsOrdered(lists[i]);
    }
    stage->floorNode()->setEnabled(state.floorEnabled);
    stage->floorNode()->transform()->setMatrix(state.floor);
    stage->fillLightNode()->transform()->setMatrix(state.fill);
    stage->restoreEditingState(state.editing);
}

bg2e::app::CommandResult AdoptEnvironmentPreset::exchange(bool applied) noexcept
{
    using namespace bg2e::app;
    if (_applied == applied)
    {
        return {};
    }
    try
    {
        auto stage = _stage.lock();
        auto root = _root.lock();
        auto* generator = _generator();
        if (!stage || !root || stage->document().root != root || !generator || !generator->acceptsState(applied ? _afterResources : _beforeResources))
        {
            return {CommandStatus::Rejected, "Prepared preset document or renderer changed"};
        }
        const auto& expected = _applied ? _after : _before;
        if (root->children().size() != expected.children.size())
        {
            return {CommandStatus::Rejected, "Prepared preset graph changed"};
        }
        for (size_t i = 0; i < expected.children.size(); ++i)
        {
            if (root->children()[i] != expected.children[i].lock())
            {
                return {CommandStatus::Rejected, "Prepared preset child order changed"};
            }
        }
        for (const auto& membership : _memberships)
        {
            auto node = membership.node.lock();
            const auto& list = _applied ? membership.after : membership.before;
            if (!node || node->orderedComponents().size() != list.size())
            {
                return {CommandStatus::Rejected, "Prepared preset membership changed"};
            }
            for (size_t i = 0; i < list.size(); ++i)
            {
                if (node->orderedComponents()[i] != list[i].lock())
                {
                    return {CommandStatus::Rejected, "Prepared preset component order changed"};
                }
            }
        }
        auto resources = applied ? _afterResources : _beforeResources;
        try
        {
            assign(applied);
        }
        catch (...)
        {
            try
            {
                assign(_applied);
            }
            catch (...)
            {
                return {CommandStatus::Faulted, "Environment preset recovery failed"};
            }
            return {CommandStatus::Failed, "Environment preset publication restored"};
        }
        generator->exchangeState(resources);
        _applied = applied;
        return {CommandStatus::Applied, {}};
    }
    catch (...)
    {
        return {CommandStatus::Rejected, "Environment preset validation failed"};
    }
}

}
