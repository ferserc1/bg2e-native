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
#include <bg2e/app/KeyEvent.hpp>

namespace bg2e {
namespace scene {

class BG2E_API InputVisitor : public NodeVisitor {
public:
    InputVisitor() {}
    
    void keyDown(Node * sceneRoot, const app::KeyEvent& event);
    void keyUp(Node * sceneRoot, const app::KeyEvent& event);
    void mouseMove(Node * sceneRoot, int x, int y);
    void mouseButtonDown(Node * sceneRoot, int button, int x, int y);
    void mouseButtonUp(Node * sceneRoot, int button, int x, int y);
    void mouseWheel(Node * sceneRoot, int deltaX, int deltaY);
    
    void visit(Node * node) override;

protected:
    enum Event {
        EventNone = 0,
        EventKeyDown = 1,
        EventKeyUp,
        EventMouseMove,
        EventButtonDown,
        EventButtonUp,
        EventWheel
    };
    
    Event _eventType = EventNone;
    int _x, _y, _button, _deltaX, _deltaY;
    const app::KeyEvent * _keyEvent;
};

}
}
