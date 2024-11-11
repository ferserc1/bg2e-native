#pragma once

#include <bg2e/render/RenderLoop.hpp>
#include <bg2e/app/InputDelegate.hpp>

namespace bg2e {
namespace app {

class Application {
public:
    virtual void init(int argc, char ** argv) = 0;

    inline std::shared_ptr<render::RenderLoopDelegate>& renderDelegate() { return _renderDelegate; }
	inline std::shared_ptr<app::InputDelegate>& inputDelegate() { return _inputDelegate; }
    
protected:
    std::shared_ptr<render::RenderLoopDelegate> _renderDelegate;
	std::shared_ptr<app::InputDelegate> _inputDelegate;
};

}
}
