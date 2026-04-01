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

#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/scene/Node.hpp>
#include <vector>

namespace bg2e {
namespace scene {

template <typename ComponentT>
class FindNodeComponentVisitor : public NodeVisitor {
public:
    const std::vector<Node*>& find(Node* sceneRoot)
    {
        _result.clear();
        sceneRoot->accept(this);
        return _result;
    }
    
    void visit(Node * node)
    {
        if (node->getComponent<ComponentT>())
        {
            _result.push_back(node);
        }
    }
    
protected:
    std::vector<Node*> _result;
};

}
}
