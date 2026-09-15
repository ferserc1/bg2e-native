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

#pragma once

#include <bg2e/common.hpp>

#include <functional>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>

namespace bg2e {
namespace scene {
    class Component;
    class Node;
}
namespace ui {

// Inspector widget for the reflected components of a scene node. Components
// with reflection metadata remain visible even when they expose no editable
// properties, because they may provide essential scene behavior.
class BG2E_API ComponentInspector {
public:
    using ResourceChangedCallback = std::function<bool(
        scene::Component * component,
        const std::string& propertyName,
        const std::filesystem::path& previousPath,
        const std::filesystem::path& selectedPath
    )>;

    void setNode(scene::Node * node)
    {
        if (_node != node)
        {
            _node = node;
            _selectedComponentIndex = 0;
        }
    }
    scene::Node * node() const { return _node; }

    void draw();

    // Fired when any property changes or a component is removed
    void onChanged(std::function<void()> cb) { _onChanged = cb; }
    void onResourceChanged(ResourceChangedCallback cb) { _onResourceChanged = std::move(cb); }

protected:
    scene::Node * _node = nullptr;
    uint32_t _selectedComponentIndex = 0;
    std::function<void()> _onChanged;
    ResourceChangedCallback _onResourceChanged;

    void notifyChanged() const;
};

}
}
