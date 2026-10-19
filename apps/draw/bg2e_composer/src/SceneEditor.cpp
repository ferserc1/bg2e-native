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

#include "EditorUi.hpp"

#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Layout.hpp>
#include <bg2e/ui/Value.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/ui/ReflectionWidget.hpp>
#include <bg2e/reflection/Registry.hpp>
#include <typeinfo>
#include <stdexcept>
#include <bg2e/draw/RendererDeferred.hpp>

namespace bg2e_composer {

void EditorUi::drawSceneEditor(const std::shared_ptr<AppDelegate>& delegate)
{
    using namespace bg2e;
    auto document = delegate->document();
    if (!document || !document->root || !delegate->selection())
    {
        return;
    }
    const auto weak = _delegate;
    _sceneTree.setRootNode(document->root.get());
    _sceneTree.setSelectionManager(delegate->selection().get());
    _sceneTree.onSelect([weak](auto node, bool additive)
    {
        if (auto owner = weak.lock())
        {
            owner->selectTreeNode(node->shared_from_this(), additive);
        }
    });
    _sceneTree.onReparent([weak](auto node, auto parent)
    {
        if (auto owner = weak.lock())
        {
            owner->reparentNode(node->shared_from_this(), parent->shared_from_this());
        }
    });
    const float treeHeight = ui::Layout::getContentRegionAvailHeight() * 0.5f;
    ui::Layout::beginChild("scene_tree", 0.0f, treeHeight);
    _sceneTree.draw();
    ui::Layout::endChild();
    ui::Layout::beginChild("node_editor");
    auto target = delegate->selection()->selectedNode();
    if (delegate->selection()->selectedNodes().size() > 1)
    {
        ui::Text::text("<multiple_selection>");
        ui::Layout::endChild();
        return;
    }
    if (!target)
    {
        ui::Text::text("No selection");
        ui::Layout::endChild();
        return;
    }
    const auto node = std::weak_ptr<scene::Node>(target->shared_from_this());
    auto name = target->name();
    if (ui::Value::text("Node name", name))
    {
        delegate->queueNodeEdit(node, [name](auto& current)
        {
            current.setName(name);
        });
    }
    auto enabled = target->enabled();
    if (ui::Button::checkBox("Enabled", &enabled))
    {
        delegate->setNodeEnabled(target->shared_from_this(), enabled);
    }
    _componentInspector.setNode(target);
    _componentInspector.onAddRequested([weak, node](const std::string& typeName)
    {
        if (auto owner = weak.lock())
        {
            owner->addSceneComponent(node, typeName);
        }
    });
    _componentInspector.onRemoveRequested([weak, node](auto component)
    {
        if (auto owner = weak.lock())
        {
            owner->editComponent(node, component, {});
        }
    });
    _componentInspector.onDrawComponent([weak, node](auto component)
    {
        try
        {
            // Drawable resources have their own left-hand editor. Never clone
            // their GPU realization during UI drawing just to stage properties.
            if (dynamic_cast<scene::DrawableComponent*>(component.get()))
            {
                ui::Text::text("Edit drawable and materials in Model Properties.");
                return;
            }
            auto staged = component->clone();
            if (!staged || staged == component || staged->ownerNode() || typeid(*staged) != typeid(*component))
            {
                throw std::logic_error("Component editor requires an independent, exact-type clone");
            }
            if (const auto* info = reflection::TypeRegistry::get().type(component->typeName()))
            {
                if (ui::ReflectionWidget::drawProperties(staged.get(), *info))
                {
                    if (auto owner = weak.lock())
                    {
                        owner->editComponent(node, component, staged);
                    }
                }
            }
        }
        catch (const std::exception& error)
        {
            ui::Text::text(error.what());
        }
    });
    _componentInspector.draw();
    ui::Layout::endChild();
}


void AppDelegate::selectTreeNode(std::weak_ptr<bg2e::scene::Node> target, bool additive)
{
    const auto generation = document() ? document()->generation : 0;
    enqueueRendererUpdate(rendererGeneration(), [weak = weak_from_this(), target, additive, generation](auto&)
    {
        auto owner = weak.lock();
        auto node = target.lock();
        if (!owner || !node || !owner->document() || owner->document()->generation != generation ||
            !owner->_stage->contains(node.get()))
        {
            return;
        }
        auto selection = owner->_selection;
        const bool selected = selection->isSelected(node.get());
        if (!additive)
        {
            selection->deselect();
        }
        if (additive && selected)
        {
            selection->removeFromSelectedItems(node.get());
        }
        else
        {
            selection->addNodeToSelection(node.get());
        }
        owner->requestSceneFrame();
    });
}


void AppDelegate::setNodeEnabled(std::weak_ptr<bg2e::scene::Node> node, bool enabled)
{
    queueNodeEdit(node, [this, enabled](auto& target)
    {
        const bool before = target.enabled();
        target.setEnabled(enabled);
        try
        {
            _stage->validateEditingGraph();
        }
        catch (...)
        {
            target.setEnabled(before);
            throw;
        }
    }, true);
}


void AppDelegate::editComponent(std::weak_ptr<bg2e::scene::Node> node,
    std::weak_ptr<bg2e::scene::Component> original,
    std::shared_ptr<bg2e::scene::Component> replacement)
{
    queueNodeEdit(node, [this, original, replacement](auto& target)
    {
        auto old = original.lock();
        if (!old || old->ownerNode() != &target)
        {
            return;
        }
        if (replacement && (replacement->ownerNode() || typeid(*replacement) != typeid(*old)))
        {
            throw std::invalid_argument("Component replacement must be detached and retain its type");
        }
        if (replacement)
        {
            replacement->setPriority(old->priority());
            replacement->setIgnoreSerialization(old->ignoreSerialization());
        }
        target.removeComponent(old);
        try
        {
            if (replacement)
            {
                target.addComponent(replacement);
            }
            _stage->validateEditingGraph();
            if (auto environment = std::dynamic_pointer_cast<bg2e::scene::EnvironmentComponent>(replacement))
            {
                auto deferred = dynamic_cast<bg2e::draw::RendererDeferred*>(renderer());
                if (!deferred || !deferred->environmentGenerator())
                {
                    throw std::logic_error("The renderer has no environment generation service");
                }
                deferred->environmentGenerator()->synchronize(environment.get());
            }
        }
        catch (...)
        {
            if (replacement && replacement->ownerNode() == &target)
            {
                target.removeComponent(replacement);
            }
            target.addComponent(old);
            throw;
        }
        // queueNodeEdit refreshes stale submesh/component handles after rebuilding
        // tools. Keep the authored node selected while editing its inspector.
    }, true);
}


void AppDelegate::addSceneComponent(std::weak_ptr<bg2e::scene::Node> node, const std::string& typeName)
{
    queueNodeEdit(node, [this, typeName](auto& target)
    {
        for (const auto& current : target.orderedComponents())
        {
            if (current->typeName() == typeName)
            {
                return;
            }
        }
        auto component = std::shared_ptr<bg2e::scene::Component>(
            bg2e::scene::ComponentFactoryRegistry::get().createDefault(typeName));
        if (!component || component->ownerNode() || component->typeName() != typeName)
        {
            throw std::invalid_argument("Component factory must return the requested detached type");
        }
        target.addComponent(component);
        try
        {
            _stage->validateEditingGraph();
            component->loadResources(*_engine);
        }
        catch (...)
        {
            target.removeComponent(component);
            throw;
        }
    }, true);
}


void AppDelegate::addSceneItem(unsigned kind)
{
    using namespace bg2e;
    if (!document() || !document()->root || kind > 3)
    {
        return;
    }
    auto selected = _selection->selectedNode();
    auto parent = selected ? selected->shared_from_this() : document()->root;
    queueNodeEdit(parent, [this, kind](auto& target)
    {
        auto child = std::make_shared<scene::Node>(kind == 1 ? "Camera" : kind == 2 ? "Light" : kind == 3 ? "Environment" : "Group");
        child->addComponent(new scene::TransformComponent());
        if (kind == 1)
        {
            child->addComponent(new scene::CameraComponent());
        }
        else if (kind == 2)
        {
            child->addComponent(new scene::LightComponent());
        }
        else if (kind == 3)
        {
            child->addComponent(new scene::EnvironmentComponent());
        }
        target.addChild(child);
        try
        {
            _stage->validateEditingGraph();
        }
        catch (...)
        {
            target.removeChild(child);
            throw;
        }
    }, true);
}


void AppDelegate::reparentNode(std::weak_ptr<bg2e::scene::Node> target,
    std::weak_ptr<bg2e::scene::Node> destination)
{
    queueNodeEdit(target, [this, destination](auto& node)
    {
        auto parent = destination.lock();
        if (!parent || !_stage->contains(parent.get()) || !node.parent() || &node == document()->root.get())
        {
            return;
        }
        for (auto ancestor = parent.get(); ancestor; ancestor = ancestor->parent())
        {
            if (ancestor == &node)
            {
                throw std::invalid_argument("Cannot reparent a node into its own subtree");
            }
        }
        auto oldParent = node.parent()->shared_from_this();
        auto retained = node.shared_from_this();
        oldParent->removeChild(retained);
        try
        {
            parent->addChild(retained);
            _stage->validateEditingGraph();
        }
        catch (...)
        {
            if (node.parent() == parent.get())
            {
                parent->removeChild(retained);
            }
            oldParent->addChild(retained);
            throw;
        }
        _selection->deselect();
        _selection->addNodeToSelection(&node);
    }, true);
}

}
