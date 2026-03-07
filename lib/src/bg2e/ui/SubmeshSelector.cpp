//
//  SubmeshSelector.cpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 31/10/25.
//

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
        _currentSelection.node = nullptr;
        _currentSelection.drawable = nullptr;
        _currentSelection.mesh = nullptr;
        _currentSelection.submesh = -1;
        for (auto item : _selectionMgr->selectedItems())
        {
            if (item->drawable)
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
    if (_currentSelection.drawable)
    {
        return _currentSelection.drawable->drawable();
    }
    return std::shared_ptr<scene::Drawable>();
}

std::shared_ptr<scene::Drawable> SubmeshSelector::editDrawable() const
{
    if (_currentSelection.drawable)
    {
        return _currentSelection.drawable->drawable();
    }
    return std::shared_ptr<scene::Drawable>();
}

int32_t SubmeshSelector::selectedItem() const
{
    if (_currentSelection.drawable)
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
            if (item->mesh == curDrawable.get())
            {
                selectedItems.push_back(item->submesh);
            }
        }
    }

    return selectedItems;
}

void SubmeshSelector::addSelectedItem(uint32_t index) const
{
    if (_currentSelection.node &&
        !_selectionMgr->isSelected(
            _currentSelection.node,
            _currentSelection.drawable,
            index
        )
    )
    {
        _selectionMgr->addToSelectedItems(
            _currentSelection.node,
            _currentSelection.drawable,
            index
        );
    }
}

bool SubmeshSelector::draw()
{
    if (!_currentSelection.drawable || !_currentSelection.mesh)
    {
        return false;
    }

    auto drawable = _currentSelection.mesh;
    auto drawableName = drawable->name();
    if (drawableName.empty())
    {
        drawableName = "Drawable";
    }


    BasicWidgets::separator("Submeshes");
    SelectableList::beginList(1);
    bool changed = false;
    for (uint32_t i = 0; i < drawable->submeshesCount(); ++i)
    {
        auto submeshName = std::to_string(i) + " - " + drawable->submeshName(i);
        bool selected = _selectionMgr->isSelected(_currentSelection.node, _currentSelection.drawable, i);
        if (SelectableList::item(submeshName, selected))
        {
            if (_selectionMgr->isSelected(_currentSelection.node, _currentSelection.drawable, i))
            {
                _selectionMgr->removeFromSelectedItems(_currentSelection.node, i);
            }
            else
            {
                _selectionMgr->addToSelectedItems(_currentSelection.node, _currentSelection.drawable, i);
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
