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

#include "AppDelegate.hpp"
#include "command/scene/StructureCommands.hpp"
#include "command/component/ComponentCommands.hpp"

#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <stdexcept>
#include <bg2e/app/CompositeCommand.hpp>

namespace bg2e_composer {
namespace {
std::vector<std::shared_ptr<bg2e::scene::Node>> lockTargets(const std::vector<std::weak_ptr<bg2e::scene::Node>>& targets)
{
    std::vector<std::shared_ptr<bg2e::scene::Node>> result;
    for (const auto& weak : targets)
    {
        auto node = weak.lock();
        if (!node)
        {
            throw std::invalid_argument("A selected scene branch expired");
        }
        result.push_back(std::move(node));
    }
    return result;
}
}

void AppDelegate::duplicateSelection()
{
    const auto targets = selectedSceneBranches();
    queueSceneCommand([this, targets]() -> std::unique_ptr<bg2e::app::Command>
    {
        auto originals = lockTargets(targets);
        if (originals.empty())
        {
            return {};
        }
        // Preserve each original's parent, including mixed-parent selections,
        // in one composite command/history entry.
        std::vector<std::unique_ptr<bg2e::app::Command>> children;
        for (const auto& original : originals)
        {
            bool nested = false;
            for (auto* ancestor = original->parent(); ancestor; ancestor = ancestor->parent())
            {
                for (const auto& other : originals)
                {
                    nested = nested || ancestor == other.get();
                }
            }
            if (nested)
            {
                continue;
            }
            if (!_stage->contains(original.get()) || original == document()->root || !original->parent())
            {
                throw std::invalid_argument("Select child branches to duplicate");
            }
            command::requireSupportedTree(original);
            auto copy = original->clone();
            try
            {
                copy->setName(original->name() + " copy");
                _stage->prepareCommandCandidate(copy);
                _stage->retainForCommand(copy);
            }
            catch (...)
            {
                _stage->discardCandidate(copy);
                throw;
            }
            children.push_back(std::make_unique<command::scene::EditNodes>(_stage, _selection,
                command::scene::NodeEdit::Insert, std::vector{copy}, original->parent()->shared_from_this(), "Duplicate node"));
        }
        return std::make_unique<bg2e::app::CompositeCommand>(bg2e::app::CommandInfo{"Duplicate selection", "scene.structure"}, std::move(children));
    });
}

void AppDelegate::removeSelection()
{
    const auto targets = selectedSceneBranches();
    queueSceneCommand([this, targets]
    {
        return std::make_unique<command::scene::EditNodes>(_stage, _selection,
            command::scene::NodeEdit::Remove, lockTargets(targets), std::weak_ptr<bg2e::scene::Node>{}, "Remove selection");
    });
}

void AppDelegate::reparentNode(std::weak_ptr<bg2e::scene::Node> node, std::weak_ptr<bg2e::scene::Node> parent)
{
    queueSceneCommand([this, node, parent]
    {
        return std::make_unique<command::scene::EditNodes>(_stage, _selection,
            command::scene::NodeEdit::Reparent, lockTargets({node}), parent, "Reparent node");
    });
}

void AppDelegate::addSceneItem(unsigned kind)
{
    if (!document() || kind > 3)
    {
        return;
    }
    auto* selected = _selection->selectedNode();
    const auto parent = std::weak_ptr<bg2e::scene::Node>(selected ? selected->shared_from_this() : document()->root);
    queueSceneCommand([this, parent, kind]
    {
        using namespace bg2e::scene;
        auto child = std::make_shared<Node>(kind == 1 ? "Camera" : kind == 2 ? "Light" : kind == 3 ? "Environment" : "Group");
        child->addComponent(std::make_shared<TransformComponent>());
        if (kind == 1)
        {
            auto camera = std::make_shared<CameraComponent>();
            camera->setProjection(_stage->camera()->projection()->clone());
            child->addComponent(camera);
        }
        else if (kind == 2)
        {
            child->addComponent(std::make_shared<LightComponent>());
        }
        else if (kind == 3)
        {
            child->addComponent(std::make_shared<EnvironmentComponent>());
        }
        try
        {
            _stage->prepareCommandCandidate(child);
            _stage->retainForCommand(child);
        }
        catch (...)
        {
            _stage->discardCandidate(child);
            throw;
        }
        return std::make_unique<command::scene::EditNodes>(_stage, _selection,
            command::scene::NodeEdit::Insert, std::vector{child}, parent, "Create scene node");
    });
}

void AppDelegate::addPrimitive(unsigned kind)
{
    if (!document() || kind > 2)
    {
        return;
    }
    auto* selected = _selection->selectedNode();
    const auto parent = std::weak_ptr<bg2e::scene::Node>(selected ? selected->shared_from_this() : document()->root);
    queueSceneCommand([this, parent, kind]
    {
        auto child = _stage->createPrimitive(kind);
        try
        {
            _stage->prepareCommandCandidate(child);
            _stage->retainForCommand(child);
        }
        catch (...)
        {
            _stage->discardCandidate(child);
            throw;
        }
        return std::make_unique<command::scene::EditNodes>(_stage, _selection,
            command::scene::NodeEdit::Insert, std::vector{child}, parent, "Create primitive");
    });
}

void AppDelegate::editComponent(std::weak_ptr<bg2e::scene::Node> node,
    std::weak_ptr<bg2e::scene::Component> original, std::shared_ptr<bg2e::scene::Component> replacement)
{
    queueSceneCommand([this, node, original, replacement]
    {
        auto old = original.lock();
        if (!old)
        {
            throw std::invalid_argument("The component to edit expired");
        }
        if (replacement)
        {
            if (!command::supportedComponent(*replacement) || replacement->ownerNode())
            {
                throw std::invalid_argument("Component replacement requires a supported detached instance");
            }
            if (auto drawable = std::dynamic_pointer_cast<bg2e::scene::DrawableComponent>(replacement); drawable && drawable->drawable())
            {
                bool aliases = false;
                const auto inspect = [&](const auto& self, const std::shared_ptr<bg2e::scene::Node>& current) -> void
                {
                    auto* live = current->drawable();
                    aliases = aliases || (live && live->drawable() &&
                        (live->drawable() == drawable->drawable() || live->drawable()->mesh() == drawable->drawable()->mesh()));
                    for (const auto& child : current->children())
                    {
                        self(self, child);
                    }
                };
                inspect(inspect, document()->root);
                if (aliases)
                {
                    drawable->setDrawable(drawable->drawable()->clone());
                }
            }
            auto lease = _stage->retainForCommand(replacement);
            if (lease.use_count() != 2)
            {
                throw std::invalid_argument("Replacement resources are already retained by history");
            }
            replacement->loadResources(*_engine);
            lease->refreshBudget();
        }
        return std::make_unique<command::component::EditMembership>(_stage, _selection, node, old, replacement);
    });
}

void AppDelegate::addSceneComponent(std::weak_ptr<bg2e::scene::Node> node, const std::string& typeName)
{
    queueSceneCommand([this, node, typeName]
    {
        auto& registry = bg2e::scene::ComponentFactoryRegistry::get();
        if (registry.owner(typeName) != 0)
        {
            throw std::invalid_argument("Plugin components need a retained-resource restoration adapter");
        }
        auto component = std::shared_ptr<bg2e::scene::Component>(registry.createDefault(typeName));
        if (!component || !command::supportedComponent(*component) || component->typeName() != typeName || component->ownerNode())
        {
            throw std::invalid_argument("The component needs a supported reversible editor adapter");
        }
        if (auto camera = std::dynamic_pointer_cast<bg2e::scene::CameraComponent>(component); camera && !camera->projection())
        {
            camera->setProjection(_stage->camera()->projection()->clone());
        }
        auto lease = _stage->retainForCommand(component);
        component->loadResources(*_engine);
        lease->refreshBudget();
        return std::make_unique<command::component::EditMembership>(_stage, _selection, node, nullptr, component);
    });
}

}
