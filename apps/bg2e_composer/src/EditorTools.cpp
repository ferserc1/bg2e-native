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

#include <bg2e/app/FileDialog.hpp>
#include <bg2e/app/Keyboard.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>
#include <bg2e/ui/SelectableList.hpp>

namespace bg2e_composer {

void EditorUi::bindDocument(const std::shared_ptr<AppDelegate>& delegate)
{
    auto document = delegate->document();
    if (!document || _boundGeneration == document->generation || !delegate->selection())
    {
        return;
    }
    if (_metadataEdit)
    {
        _metadataEdit->cancel();
    }
    _boundGeneration = document->generation;
    const auto generation = _boundGeneration;
    const auto weak = _delegate;
    _componentInspector.onEditRequested([weak, generation](const auto& component, auto phase, uint64_t, const auto& intent)
    {
        if (phase == bg2e::ui::EditPhase::Cancel)
        {
            return true;
        }
        auto owner = weak.lock();
        return owner && owner->document() && owner->document()->generation == generation &&
            owner->componentEditIntent(component, phase, intent);
    });
    _componentInspector.onActionRequested([weak, generation](const auto& component, const auto& intent)
    {
        if (auto owner = weak.lock(); owner && owner->document() && owner->document()->generation == generation)
        {
            owner->componentAction(component, intent);
        }
    });
    _metadataEdit = std::make_unique<bg2e::ui::EditSession>(
        [weak, generation](auto phase, uint64_t, const auto&, const std::any& value)
        {
            if (phase == bg2e::ui::EditPhase::Cancel)
            {
                return true;
            }
            auto owner = weak.lock();
            if (!owner || !owner->document() || owner->document()->generation != generation)
            {
                return false;
            }
            if (phase == bg2e::ui::EditPhase::Commit)
            {
                try
                {
                    return owner->editSubmeshMetadata(std::any_cast<const command::scene::MetadataRequest&>(value)).accepted();
                }
                catch (...)
                {
                    return false;
                }
            }
            return true;
        });
    _metadataEdit->beginFrame();
    _material.setSelectionManager(delegate->selection());
    _material.setEditGuard([weak, generation]
    {
        auto owner = weak.lock();
        return owner && owner->document() && owner->document()->generation == generation &&
            owner->document()->commands && !owner->document()->commands->history().faulted() &&
            !owner->document()->commands->history().transactionActive();
    });
    _material.onChanged([weak](auto& renderer)
    {
        if (auto owner = weak.lock())
        {
            owner->notifyEdited(renderer);
        }
    });
    _material.setCpuValueEditsEnabled(true);
    _material.setRetainedResourceEditsEnabled(true);
}

void EditorUi::drawTools(const std::shared_ptr<AppDelegate>& delegate)
{
    using namespace bg2e;
    const auto document = delegate->document();
    if (!document || !document->root)
    {
        ui::Text::text("Open a model to edit it.");
        return;
    }
    auto selection = delegate->selection();
    auto target = selection->selectedNode();
    if (selection->selectedNodes().size() > 1)
    {
        if (_ui)
        {
            _material.draw(*_ui);
        }
        return;
    }
    if (!target || !target->drawable() || !target->drawable()->drawable())
    {
        ui::Text::text("No drawable selected");
        return;
    }
    auto* component = target->drawable();
    auto drawable = component->drawable();
    ui::Text::text(target->name());
    ui::Text::separator("Submeshes");
    if (ui::SelectableList::beginList())
    {
        for (uint32_t sub = 0; sub < drawable->submeshesCount(); ++sub)
        {
            bool selected = selection->isSelected(target, component, sub);
            if (ui::SelectableList::item(drawable->submeshName(sub) + "##sub" + std::to_string(sub), selected))
            {
                delegate->selectNode(target->shared_from_this(), sub, app::Keyboard::shiftPressed());
            }
        }
        ui::SelectableList::endList();
    }
    if (auto item = selection->selectedItem(); item && item->lockMesh())
    {
        auto mesh = item->lockMesh();
        auto name = mesh->submeshName(item->submesh);
        auto group = mesh->submeshGroupName(item->submesh);
        auto visible = mesh->submeshVisibility(item->submesh);
        const auto drawMetadata = [&](command::scene::MetadataField field, const char* label, bool all)
        {
            command::scene::MetadataRequest request;
            request.field = field;
            request.text = field == command::scene::MetadataField::Name ? name : group;
            request.visible = visible;
            const auto targets = all ? selection->selectedItems() : std::vector{item};
            for (const auto& target : targets)
            {
                request.targets.push_back(*target);
            }
            const auto key = std::to_string(reinterpret_cast<uintptr_t>(mesh.get())) + ":" +
                std::to_string(item->submesh) + ":" + std::to_string(static_cast<int>(field));
            request = std::any_cast<command::scene::MetadataRequest>(_metadataEdit->value(key, std::move(request)));
            bool changed = false;
            const auto state = ui::EditInteraction::draw([&]
            {
                if (field == command::scene::MetadataField::Visibility)
                {
                    changed = ui::Button::checkBox(label, &request.visible);
                }
                else if (request.text.size() < 4096)
                {
                    changed = ui::Value::text(label, request.text, 4096);
                }
                else
                {
                    ui::Text::text("Metadata is too long to edit");
                }
            });
            _metadataEdit->submit(key, std::move(request), changed, state);
        };
        drawMetadata(command::scene::MetadataField::Name, "Submesh name", false);
        drawMetadata(command::scene::MetadataField::Group, "Group", true);
        drawMetadata(command::scene::MetadataField::Visibility, "Visible", true);
        ui::Text::separator("Geometry (whole selected meshes)");
        const char* operations[]{"Z up to Y up", "Center geometry", "Put on floor", "cm to m"};
        for (unsigned i = 0; i < 4; ++i)
        {
            if (ui::Button::button(operations[i]))
            {
                delegate->editGeometry(i);
            }
        }
        if (ui::Button::button("Copy material"))
        {
            delegate->copyMaterial();
        }
        if (ui::Button::button("Paste material to selection"))
        {
            delegate->pasteMaterial();
        }
        if (ui::Button::button("Apply material JSON patch..."))
        {
            delegate->applyMaterialPatch(app::FileDialog::getOpenFilePath({{"Partial material", "json"}}));
        }
        ui::Text::separator("Material and textures");
        if (_ui)
        {
            _material.draw(*_ui);
        }
    }
}

}
