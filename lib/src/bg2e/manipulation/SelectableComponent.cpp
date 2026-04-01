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

#include <bg2e/manipulation/SelectableComponent.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Drawable.hpp>

namespace bg2e::manipulation {

SelectableComponent::SelectableComponent()
{
}

void SelectableComponent::update(float)
{
    if (!ownerNode())
    {
        return;
    }
    
    auto drwComponent = ownerNode()->drawable();
    auto drw = drwComponent ? dynamic_cast<scene::Drawable*>(drwComponent->drawableBase().get()) : nullptr;
    if (_submeshCount == 0 && drw)
    {
        _submeshCount = drw->submeshesCount();
        _submeshSelected.resize(_submeshCount);
        for (uint32_t i = 0; i < _submeshCount; ++i)
        {
            _identifier[i] = generateIdentifier();
            _submeshSelected[i] = false;
        }
    } 
}

uint32_t SelectableComponent::_lastIdentifier = 0;

uint32_t SelectableComponent::generateIdentifier()
{
    SelectableComponent::_lastIdentifier++;
    return SelectableComponent::_lastIdentifier;
}


scene::BG2E_SCENE_REGISTER_COMPONENT(SelectableComponent);

}
