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
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/scene/OrbitCameraComponent.hpp>
#include <bg2e/scene/PolarTransformController.hpp>
#include <bg2e/scene/FixedScaleTransformController.hpp>
#include <bg2e/scene/ChainJoint.hpp>

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
    if (!_nameEdit || _nameEditNode.lock().get() != target || _nameEditGeneration != document->generation)
    {
        if (_nameEdit)
        {
            _nameEdit->cancel();
        }
        _nameEditNode = target->shared_from_this();
        _nameEditGeneration = document->generation;
        _nameEdit = std::make_unique<ui::EditSession>(
            [weak, node, generation = document->generation](ui::EditPhase phase, uint64_t, const auto&, const std::any& value)
            {
                auto owner = weak.lock();
                if (phase == ui::EditPhase::Cancel)
                {
                    return true; // Only a transient UI draft exists until Commit.
                }
                auto target = node.lock();
                if (!owner || !target || !owner->document() || owner->document()->generation != generation ||
                    (!owner->stage()->contains(target.get()) && target != owner->stage()->fillLightNode()))
                {
                    return false;
                }
                if (phase == ui::EditPhase::Begin && owner->document()->commands->history().transactionActive())
                {
                    return false;
                }
                if (phase == ui::EditPhase::Commit)
                {
                    try
                    {
                        return owner->renameNode(node, std::any_cast<const std::string&>(value)).accepted();
                    }
                    catch (...)
                    {
                        return false;
                    }
                }
                return true;
            });
        _nameEdit->beginFrame();
    }
    auto name = std::any_cast<std::string>(_nameEdit->value("name", target->name()));
    bool nameChanged = false;
    const auto nameState = ui::EditInteraction::draw([&]
    {
        if (name.size() <= size_t(std::numeric_limits<int>::max() - 256))
        {
            const auto capacity = std::max(size_t(200), name.size() + 256);
            nameChanged = ui::Value::text("Node name", name, static_cast<int>(capacity));
        }
        else
        {
            ui::Text::text("Node name is too long to edit");
        }
    });
    _nameEdit->submit("name", std::move(name), nameChanged, nameState);
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
    _componentInspector.onDrawComponent({});
    _componentInspectorDrawn = true;
    _componentInspector.draw();
    ui::Layout::endChild();
}


}
