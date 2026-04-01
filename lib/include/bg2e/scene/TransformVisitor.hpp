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

#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/scene/Node.hpp>

namespace bg2e {
namespace scene {

class TransformVisitor : public NodeVisitor {
public:

    static glm::mat4 getWorldMatrix(Node * node)
    {
        if (!node)
        {
            return glm::mat4{ 1.0f };
        }
        
        TransformVisitor visitor;
        node->acceptReverse(&visitor);
        return visitor._worldMatrix;
    }
    
    void visit(Node * node)
    {
        auto transform = node->getComponent<TransformComponent>();
        if (transform)
        {
            _worldMatrix = _worldMatrix * transform->matrix();
        }
    }
    
protected:
    glm::mat4 _worldMatrix { 1.0f };
};

}
}
