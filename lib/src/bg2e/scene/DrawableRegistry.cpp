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

#include <bg2e/scene/DrawableRegistry.hpp>
#include <bg2e/utils/utils.hpp>

namespace bg2e::scene {

bool DrawableRegistry::nameExists(const std::string& name) const
{
    for (const auto& d : _drawables)
    {
        if (d->name() == name)
            return true;
    }
    return false;
}

void DrawableRegistry::registerDrawable(std::shared_ptr<DrawableBase> drawable)
{
    for (const auto& d : _drawables)
    {
        if (d.get() == drawable.get())
            return;
    }

    std::string name = drawable->name();
    if (name.empty())
    {
        name = utils::uniqueId();
    }

    if (nameExists(name))
    {
        int suffix = 1;
        std::string baseName = name;
        while (nameExists(baseName + "_" + std::to_string(suffix)))
        {
            ++suffix;
        }
        name = baseName + "_" + std::to_string(suffix);
    }

    drawable->setName(name);
    _drawables.push_back(drawable);
}

void DrawableRegistry::cleanup()
{
    _drawables.clear();
}

}
