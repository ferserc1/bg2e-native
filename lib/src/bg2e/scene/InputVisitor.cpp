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

#include <bg2e/scene/InputVisitor.hpp>

namespace bg2e::scene {

void InputVisitor::keyDown(Node * sceneRoot, const app::KeyEvent& event)
{
    _eventType = EventKeyDown;
    _keyEvent = &event;
    
    sceneRoot->accept(this);
}

void InputVisitor::keyUp(Node * sceneRoot, const app::KeyEvent& event)
{
    _eventType = EventKeyUp;
    _keyEvent = &event;
    
    sceneRoot->accept(this);
}

void InputVisitor::mouseMove(Node * sceneRoot, int x, int y)
{
    _eventType = EventMouseMove;
    _x = x;
    _y = y;
    
    sceneRoot->accept(this);
}

void InputVisitor::mouseButtonDown(Node * sceneRoot, int button, int x, int y)
{
    _eventType = EventButtonDown;
    _button = button;
    _x = x;
    _y = y;
    
    sceneRoot->accept(this);
}

void InputVisitor::mouseButtonUp(Node * sceneRoot, int button, int x, int y)
{
    _eventType = EventButtonUp;
    _button = button;
    _x = x;
    _y = y;
    
    sceneRoot->accept(this);
}

void InputVisitor::mouseWheel(Node * sceneRoot, int deltaX, int deltaY)
{
    _eventType = EventWheel;
    _deltaX = deltaX;
    _deltaY = deltaY;
    
    sceneRoot->accept(this);
}

void InputVisitor::visit(Node * node)
{
    for (auto &comp : node->orderedComponents())
    {
        switch (_eventType) {
        case EventNone:
            break;
        case EventKeyDown:
            comp->keyDown(*_keyEvent);
            break;
        case EventKeyUp:
            comp->keyUp(*_keyEvent);
            break;
        case EventMouseMove:
            comp->mouseMove(_x, _y);
            break;
        case EventButtonDown:
            comp->mouseButtonDown(_button, _x, _y);
            break;
        case EventButtonUp:
            comp->mouseButtonUp(_button, _x, _y);
            break;
        case EventWheel:
            comp->mouseWheel(_deltaX, _deltaY);
            break;
        }
    }
}

}
