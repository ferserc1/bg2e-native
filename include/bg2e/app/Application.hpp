#pragma once

#include <bg2e/render/RenderLoopDelegate.hpp>

namespace bg2e {
namespace app {

class Application {
public:
    virtual void init(int argc, char ** argv) = 0;

    inline render::RenderLoopDelegate * renderDelegate() { return _renderDelegate.get(); }
    
protected:
    std::shared_ptr<render::RenderLoopDelegate> _renderDelegate;
};

}
}
