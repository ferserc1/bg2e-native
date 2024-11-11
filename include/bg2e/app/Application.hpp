#pragma once

#include <bg2e/render/RenderLoop.hpp>

namespace bg2e {
namespace app {

class Application {
public:
    virtual void init(int argc, char ** argv) = 0;

    inline std::shared_ptr<render::RenderLoopDelegate>& renderDelegate() { return _renderDelegate; }
    
protected:
    std::shared_ptr<render::RenderLoopDelegate> _renderDelegate;
};

}
}
