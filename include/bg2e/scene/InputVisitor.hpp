//
//  InputVisitor.hpp
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
