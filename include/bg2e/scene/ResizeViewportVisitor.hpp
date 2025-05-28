//
//  ResizeViewportVisitor.hpp
#pragma once

#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/math/projections.hpp>
#include <bg2e/render/Vulkan.hpp>

namespace bg2e {
namespace scene {

class BG2E_API ResizeViewportVisitor : public NodeVisitor {
public:
    ResizeViewportVisitor() :_vp{800, 600} {}
    
    void resizeViewport(Node * sceneRoot, const math::Viewport& vp);
    void resizeViewport(Node * sceneRoot, const VkExtent2D& viewportExtent);
    
    void visit(Node *) override;

protected:
    math::Viewport _vp;
};

}
}
