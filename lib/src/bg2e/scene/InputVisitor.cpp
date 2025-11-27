//
//  InputVisitor.cpp
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
    for (auto &comp : node->components())
    {
        switch (_eventType) {
        case EventNone:
            break;
        case EventKeyDown:
            comp.second->keyDown(*_keyEvent);
            break;
        case EventKeyUp:
            comp.second->keyUp(*_keyEvent);
            break;
        case EventMouseMove:
            comp.second->mouseMove(_x, _y);
            break;
        case EventButtonDown:
            comp.second->mouseButtonDown(_button, _x, _y);
            break;
        case EventButtonUp:
            comp.second->mouseButtonUp(_button, _x, _y);
            break;
        case EventWheel:
            comp.second->mouseWheel(_deltaX, _deltaY);
            break;
        }
    }
}

}
