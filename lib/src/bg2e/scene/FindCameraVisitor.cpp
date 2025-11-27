//
//  FindCameraVisitor.cpp

#include <bg2e/scene/FindCameraVisitor.hpp>
#include <bg2e/scene/Node.hpp>

namespace bg2e::scene {

void FindCameraVisitor::findCameras(Node * node)
{
    cleanup();
    node->accept(this);
}

void FindCameraVisitor::visit(Node * node)
{
    auto camera = node->camera();
    if (camera)
    {
        _cameras.push_back(camera);
    }
}

void FindCameraVisitor::cleanup()
{
    _cameras.clear();
}

}
