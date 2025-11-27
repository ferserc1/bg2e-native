#pragma once

#include <bg2e/render/RenderLoop.hpp>
#include <bg2e/app/InputDelegate.hpp>
#include <bg2e/ui/UserInterface.hpp>

namespace bg2e {
namespace app {

class Application {
public:
    virtual void init(int argc, char ** argv) = 0;

    inline std::shared_ptr<render::RenderLoopDelegate>& renderDelegate() { return _renderDelegate; }
	inline std::shared_ptr<app::InputDelegate>& inputDelegate() { return _inputDelegate; }
	inline std::shared_ptr<ui::UserInterfaceDelegate>& uiDelegate() { return _uiDelegate; }
	inline void setRenderDelegate(std::shared_ptr<render::RenderLoopDelegate> delegate) { _renderDelegate = delegate; }
	inline void setInputDelegate(std::shared_ptr<app::InputDelegate> delegate) { _inputDelegate = delegate; }
	inline void setUiDelegate(std::shared_ptr<ui::UserInterfaceDelegate> delegate) { _uiDelegate = delegate; }
    
protected:
    std::shared_ptr<render::RenderLoopDelegate> _renderDelegate;
	std::shared_ptr<app::InputDelegate> _inputDelegate;
	std::shared_ptr<ui::UserInterfaceDelegate> _uiDelegate;
};

}
}
