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

void SubmeshSelector::setEditDrawable(std::shared_ptr<scene::Drawable> drawable)
{
    clearWidgets();
    _drawable = drawable;
    initWidgets();
}

void SubmeshSelector::clearDrawable()
{
    _drawable.reset();
    clearWidgets();
}

void SubmeshSelector::clearSelection()
{
    _selectedItems.clear();
    for (size_t i = 0; i < _selectedSubmeshes.size(); ++i)
    {
        _selectedSubmeshes[i] = false;
    }
}

void SubmeshSelector::addSelectedItem(uint32_t index)
{
    if (index < _selectedSubmeshes.size())
    {
        _selectedSubmeshes[index] = true;
        if (std::find(_selectedItems.begin(), _selectedItems.end(), index) == _selectedItems.end())
        {
            _selectedItems.push_back(index);
        }
    }
}

bool SubmeshSelector::draw()
{
    auto drawableName = _drawable.get() ? _drawable->name() : "";
    if (drawableName.empty())
    {
        drawableName = "Drawable";
    }
    if (_drawable.get())
    {
        BasicWidgets::separator("Submeshes");
        SelectableList::beginList(2);
        auto changed = false;
        for (uint32_t i = 0; i < _drawable->submeshesCount(); ++i)
        {
            auto submeshName = std::to_string(i) + " - " + _drawable->submeshName(i);
            bool selected = _selectedSubmeshes[i];
            if (SelectableList::item(submeshName, selected))
            {
                clearSelection();
                _selectedSubmeshes[i] = true;
                _selectedItems.push_back(i);
                changed = true;
            }
            std::string buttonText = selected ? "Deselect" : "Select";
            if (SelectableList::itemButton(buttonText + "##" + std::to_string(i)))
            {
                selected = !selected;
                _selectedSubmeshes[i] = selected;
                auto itemIterator = std::ranges::find(_selectedItems, i);
                if (selected && itemIterator == _selectedItems.end())
                {
                    _selectedItems.push_back(i);
                }
                if (!selected && itemIterator != _selectedItems.end())
                {
                    _selectedItems.erase(itemIterator);
                }
                changed = true;
            }
        }
        SelectableList::endList();
        if (BasicWidgets::button("Clear Selection"))
        {
            clearSelection();
            changed = true;
        }
        return changed;
    }
    return false;
}

void SubmeshSelector::cleanup()
{
    _drawable.reset();
    clearWidgets();
}

void SubmeshSelector::initWidgets()
{
    if (!_drawable.get())
    {
        return;
    }
    
    for (uint32_t i = 0; i < _drawable->submeshesCount(); ++i)
    {
        _selectedSubmeshes.push_back(false);
    }
}

void SubmeshSelector::clearWidgets()
{
    _selectedSubmeshes.clear();
    _selectedItems.clear();
}

}
