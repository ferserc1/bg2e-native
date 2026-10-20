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
    _boundGeneration = document->generation;
    const auto generation = _boundGeneration;
    const auto weak = _delegate;
    _material.setSelectionManager(delegate->selection());
    _material.setEditGuard([weak, generation]
    {
        auto owner = weak.lock();
        return owner && owner->document() && owner->document()->generation == generation;
    });
    _material.onChanged([weak](auto& renderer)
    {
        if (auto owner = weak.lock())
        {
            owner->notifyEdited(renderer);
        }
    });
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
    if (!target || !target->drawable() || !target->drawable()->drawDrawable())
    {
        ui::Text::text("No drawable selected");
        return;
    }
    auto* component = target->drawable();
    auto drawable = component->drawDrawable();
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
    if (auto item = selection->selectedItem(); item && item->lockDrawMesh())
    {
        auto mesh = item->lockDrawMesh();
        auto name = mesh->submeshName(item->submesh);
        auto group = mesh->submeshGroupName(item->submesh);
        auto visible = mesh->submeshVisibility(item->submesh);
        auto editMetadata = [&](const auto& operation, bool all = false)
        {
            const auto targets = all ? selection->selectedItems() : std::vector<std::shared_ptr<manipulation::SelectionItem>>{item};
            for (const auto& current : targets)
            {
                const auto snapshot = *current;
                delegate->queueNodeEdit(current->node, [snapshot, operation](auto&)
                {
                    if (auto mesh = snapshot.lockDrawMesh())
                    {
                        operation(*mesh, snapshot.submesh);
                    }
                });
            }
        };
        if (ui::Value::text("Submesh name", name))
        {
            editMetadata([name](auto& mesh, auto sub)
            {
                mesh.setSubmeshName(name, sub);
            });
        }
        if (ui::Value::text("Group", group))
        {
            editMetadata([group](auto& mesh, auto sub)
            {
                mesh.setSubmeshGroupName(group, sub);
            }, true);
        }
        if (ui::Button::checkBox("Visible", &visible))
        {
            editMetadata([visible](auto& mesh, auto sub)
            {
                mesh.setSubmeshVisibility(visible, sub);
            }, true);
        }
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
