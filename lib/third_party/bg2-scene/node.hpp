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

#ifndef _BG2_SCENE_NODE_HPP_
#define _BG2_SCENE_NODE_HPP_

#include <string>
#include <vector>

#include <component.hpp>

namespace bg2scene {

    class Node {
    public:
        Node();
        virtual ~Node();
    
        std::string name;
        bool enabled;
        bool steady;

        std::vector<Node*> children;
        std::vector<Component*> components;
    };

}

#endif
