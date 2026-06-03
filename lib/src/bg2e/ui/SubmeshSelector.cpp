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

#include <bg2e/ui/SubmeshSelector.hpp>
#include <bg2e/ui/BasicWidgets.hpp>

#include <algorithm>


namespace bg2e::ui {

SubmeshSelector::~SubmeshSelector()
{
    
}

void SubmeshSelector::init(std::shared_ptr<manipulation::SelectionManager> sm)
{
    _selectionMgr = sm;
    _selectionMgr->onSelect([&]()
    {
        _currentSelection.node.reset();
        _currentSelection.drawable.reset();
        _currentSelection.mesh.reset();
        _currentSelection.submesh = -1;
        for (auto item : _selectionMgr->selectedItems())
        {
            if (!item->drawable.expired())
            {
                _currentSelection.node = item->node;
                _currentSelection.drawable = item->drawable;
                _currentSelection.mesh = item->mesh;
                _currentSelection.submesh = item->submesh;
                break;
            }
        }
        clearWidgets();
        initWidgets();
    });
}

std::shared_ptr<scene::Drawable> SubmeshSelector::editDrawable()
{
    auto drawable = _currentSelection.drawable.lock();
    if (drawable)
    {
        return drawable->drawable();
    }
    return std::shared_ptr<scene::Drawable>();
}

std::shared_ptr<scene::Drawable> SubmeshSelector::editDrawable() const
{
    auto drawable = _currentSelection.drawable.lock();
    if (drawable)
    {
        return drawable->drawable();
    }
    return std::shared_ptr<scene::Drawable>();
}

int32_t SubmeshSelector::selectedItem() const
{
    if (!_currentSelection.drawable.expired())
    {
        return _currentSelection.submesh;
    }
    return -1;
}

std::vector<uint32_t> SubmeshSelector::selectedItems() const
{
    std::vector<uint32_t> selectedItems;
    auto curDrawable = editDrawable();

    if (curDrawable.get())
    {
        for (auto & item : _selectionMgr->selectedItems())
        {
            auto itemMesh = item->mesh.lock();
            if (itemMesh.get() == curDrawable.get())
            {
                selectedItems.push_back(item->submesh);
            }
        }
    }

    return selectedItems;
}

void SubmeshSelector::addSelectedItem(uint32_t index) const
{
    auto node = _currentSelection.nodePtr();
    auto drawable = _currentSelection.drawablePtr();
    if (node &&
        !_selectionMgr->isSelected(
            node,
            drawable,
            index
        )
    )
    {
        _selectionMgr->addToSelectedItems(
            node,
            drawable,
            index
        );
    }
}

bool SubmeshSelector::draw()
{
    auto drawableComp = _currentSelection.drawable.lock();
    auto drawableMesh = _currentSelection.mesh.lock();
    if (!drawableComp || !drawableMesh)
    {
        return false;
    }

    auto drawableName = drawableMesh->name();
    if (drawableName.empty())
    {
        drawableName = "Drawable";
    }


    BasicWidgets::separator("Submeshes");
    SelectableList::beginList(1);
    bool changed = false;
    auto node = _currentSelection.nodePtr();
    auto drawable = _currentSelection.drawablePtr();
    for (uint32_t i = 0; i < drawableMesh->submeshesCount(); ++i)
    {
        auto submeshName = std::to_string(i) + " - " + drawableMesh->submeshName(i);
        bool selected = _selectionMgr->isSelected(node, drawable, i);
        if (SelectableList::item(submeshName, selected))
        {
            if (_selectionMgr->isSelected(node, drawable, i))
            {
                _selectionMgr->removeFromSelectedItems(node, i);
            }
            else
            {
                _selectionMgr->addToSelectedItems(node, drawable, i);
            }
            changed = true;
        }
    }
    SelectableList::endList();
    if (BasicWidgets::button("Clear Selection"))
    {
        _selectionMgr->deselect();
        changed = true;
    }
    return changed;
}

void SubmeshSelector::cleanup()
{
    clearWidgets();
}

void SubmeshSelector::initWidgets()
{


}

void SubmeshSelector::clearWidgets()
{

}

}
