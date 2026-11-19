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
#include <utility>

namespace bg2e_composer {

void EditorUi::initDrawableBrowser()
{
    const auto weak = _delegate;
    _drawableInstances.configure([weak]() -> const bg2e::ui::DrawableBrowserSnapshot*
    {
        const auto owner = weak.lock();
        return owner ? &owner->drawableBrowserSnapshot() : nullptr;
    }, [weak]
    {
        const auto owner = weak.lock();
        return owner && owner->sceneResourceOperationAvailable();
    }, [weak]
    {
        if (auto owner = weak.lock())
        {
            owner->loadDrawableIntoCache();
        }
    }, [weak](auto handle)
    {
        if (auto owner = weak.lock())
        {
            owner->unloadCachedDrawable(std::move(handle));
        }
    }, [weak](auto handle)
    {
        if (auto owner = weak.lock())
        {
            owner->createDrawableInstance(std::move(handle));
        }
    }, [weak]
    {
        if (auto owner = weak.lock())
        {
            owner->createDrawableInstanceFromOwner();
        }
    }, [weak](auto handle)
    {
        if (auto owner = weak.lock())
        {
            owner->rebindSelectedDrawableInstance(std::move(handle));
        }
    }, [weak]
    {
        if (auto owner = weak.lock())
        {
            owner->removeSelectedDrawableInstance();
        }
    }, [weak]
    {
        const auto owner = weak.lock();
        return owner ? owner->status() : std::string{};
    });
}

}
