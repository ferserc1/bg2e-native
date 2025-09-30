//
//  Workspace.hpp

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/ui/Window.hpp>

namespace bg2e {
namespace ui {

class BG2E_API Workspace {
public:

    void setup(
        uint32_t width, uint32_t height,
        Window * toolBar,
        Window * leftPanel,
        Window * rightPanel,
        Window * bottomPanel
    );

    void resize(uint32_t width, uint32_t height);
    
    // To use the workspace draw function, the workspace windows must have
    // a draw lambda function configured
    void draw();

    inline bool isValid() const { return _viewportWidth > 0 && _viewportHeight > 0; }
    
    inline bool toolBarVisible() const { return _drawToolBar; }
    inline bool leftPanelVisible() const { return _drawLeftPanel; }
    inline bool rightPanelVisible() const { return _drawRightPanel; }
    inline bool bottomPanelVisible() const { return _drawBottomPanel; }

    inline void toggleToolBar() { _drawToolBar = !_drawToolBar; updateWindows(); }
    inline void toggleLeftPanel() { _drawLeftPanel = !_drawLeftPanel; updateWindows(); }
    inline void toggleRightPanel() { _drawRightPanel = !_drawRightPanel; updateWindows(); }
    inline void toggleBottomPanel() { _drawBottomPanel = !_drawBottomPanel; updateWindows(); }
    
    inline void showToolBar() { _drawToolBar = true; updateWindows(); }
    inline void showLeftPanel() { _drawLeftPanel = true; updateWindows(); }
    inline void showRightPanel() { _drawRightPanel = true; updateWindows(); }
    inline void showBottomPanel() { _drawBottomPanel = true; updateWindows(); }
    
    inline void hideToolBar() { _drawToolBar = false; updateWindows(); }
    inline void hideLeftPanel() { _drawLeftPanel = false; updateWindows(); }
    inline void hideRightPanel() { _drawRightPanel = false; updateWindows(); }
    inline void hideBottomPanel() { _drawBottomPanel = false; updateWindows(); }
    
    inline void setToolBarVisible(bool v) { _drawToolBar = v; updateWindows(); }
    inline void setLeftPanelVisible(bool v) { _drawLeftPanel = v; updateWindows(); }
    inline void setRightPanelVisible(bool v) { _drawRightPanel = v; updateWindows(); }
    inline void setBottomPanelVisible(bool v) { _drawBottomPanel = v; updateWindows(); }
    
    struct PanelSize
    {
        uint32_t min = 200;
        uint32_t max = 400;
        float relative = 0.2f;
    };
    
    inline PanelSize& leftPanelSize() { return _leftPanelSize; }
    inline const PanelSize& leftPanelSize() const { return _leftPanelSize; }
    
    inline PanelSize& rightPanelSize() { return _rightPanelSize; }
    inline const PanelSize& rightPanelSize() const { return _rightPanelSize; }
    
    inline PanelSize& bottomPanelSize() { return _bottomPanelSize; }
    inline const PanelSize& bottomPanelSize() const { return _bottomPanelSize; }
    
protected:
    Window * _toolBar = nullptr;
    Window * _leftPanel = nullptr;
    Window * _rightPanel = nullptr;
    Window * _bottomPanel = nullptr;
    
    bool _drawToolBar = true;
    bool _drawLeftPanel = true;
    bool _drawRightPanel = true;
    bool _drawBottomPanel = true;
    
    uint32_t _viewportWidth = 0;
    uint32_t _viewportHeight = 0;
    
    uint32_t _toolBarHeight = 50;
    
    
    
    PanelSize _leftPanelSize = {
        .min = 150,
        .max = 500,
        .relative = 0.15f
    };
    PanelSize _rightPanelSize = {
        .min = 150,
        .max = 500,
        .relative = 0.15f
    };
    PanelSize _bottomPanelSize = {
        .min = 100,
        .max = 400,
        .relative = 0.20f
    };
    
    void updateWindows();
    
    uint32_t getPanelSize(PanelSize & size, uint32_t vpSize);
};

}
}
