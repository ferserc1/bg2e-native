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
#include <bg2e/scene/Component.hpp>

#include <unordered_map>
#include <vector>
#include <algorithm>

namespace bg2e {
namespace manipulation {

class BG2E_API SelectableComponent : public scene::Component {
public:
    BG2E_COMPONENT_TYPE_NAME("Selectable");
    
    SelectableComponent();
    virtual ~SelectableComponent() = default;
    
    void update(float) override;

    inline uint32_t identifier(uint32_t submeshIndex) { return _identifier[submeshIndex]; }
    inline uint32_t submeshCount() const { return _submeshCount; }
    
    static inline uint32_t decodeObjectId(const uint8_t data[4])
    {
        return
            static_cast<uint32_t>(data[0]) |
            (static_cast<uint32_t>(data[1]) << 8) |
            (static_cast<uint32_t>(data[2]) << 16) |
            (static_cast<uint32_t>(data[3]) << 24);
    }

    inline bool isSelected(uint32_t submeshIndex) const { return _submeshSelected[submeshIndex]; }
    inline void setSelected(uint32_t submeshIndex, bool selected)
    {
        if (_submeshSelected.size() > submeshIndex)
        {
            _submeshSelected[submeshIndex] = selected;
        }
    }
    inline void unselectAll() { std::fill(_submeshSelected.begin(), _submeshSelected.end(), false); }
    
protected:
    // submeshIndex, submeshIdentifier
    std::unordered_map<uint32_t, uint32_t> _identifier;
    std::vector<bool> _submeshSelected;
    uint32_t _submeshCount = 0;
    
    static uint32_t _lastIdentifier;
    static uint32_t generateIdentifier();
};

}
}
