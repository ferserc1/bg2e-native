//
//  Workspace.cpp
#include <bg2e/ui/Workspace.hpp>

namespace bg2e::ui {

void Workspace::setup(
    uint32_t width, uint32_t height,
    Window * toolBar,
    Window * leftPanel,
    Window * rightPanel,
    Window * bottomPanel,
    Window * statusBar
) {
    _viewportWidth = width;
    _viewportHeight = height;
    _toolBar = toolBar;
    _leftPanel = leftPanel;
    _rightPanel = rightPanel;
    _bottomPanel = bottomPanel;
    _statusBar = statusBar;
    
    updateWindows();
}

void Workspace::resize(uint32_t width, uint32_t height)
{
    _viewportWidth = width;
    _viewportHeight = height;
    updateWindows();
    
}

void Workspace::draw()
{
    if (_toolBar && _drawToolBar)
    {
        _toolBar->draw();
    }
    
    if (_leftPanel && _drawLeftPanel)
    {
        _leftPanel->draw();
    }
    
    if (_rightPanel && _drawRightPanel)
    {
        _rightPanel->draw();
    }
    
    if (_bottomPanel && _drawBottomPanel)
    {
        _bottomPanel->draw();
    }

    if (_statusBar && _drawStatusBar)
    {
        _statusBar->draw();
    }
}

// void Workspace::updateWindows()
// {
//     uint32_t topPadding = 0;
//     uint32_t bottomPadding = 0;
//     if (_toolBar && _drawToolBar)
//     {
//         topPadding = _toolBarHeight;
//         _toolBar->setPosition(0, 0);
//         _toolBar->setSize(_viewportWidth, _toolBarHeight);
//         _toolBar->options = Window::Options {
//             .noTitleBar = true,
//             .noScrollbar = true,
//             .noMenu = false,
//             .noMove = true,
//             .noResize = true,
//             .noCollapse = true,
//             .noNav = true,
//             .noBringToFront = true,
//             .noClose = true
//         };
//     }
//
//     if (_bottomPanel && _drawBottomPanel)
//     {
//         bottomPadding = getPanelSize(_bottomPanelSize, _viewportHeight);
//         _bottomPanel->setPosition(0, _viewportHeight - bottomPadding);
//         _bottomPanel->setSize(_viewportWidth, bottomPadding);
//         _bottomPanel->options = Window::Options {
//             .noTitleBar = false,
//             .noScrollbar = false,
//             .noMenu = true,
//             .noMove = true,
//             .noResize = true,
//             .noCollapse = true,
//             .noNav = true,
//             .noBringToFront = true,
//             .noClose = true
//         };
//     }
//
//     if (_leftPanel && _drawLeftPanel)
//     {
//
//         _leftPanel->setPosition(0, topPadding);
//         _leftPanel->setSize(
//             getPanelSize(_leftPanelSize, _viewportWidth),
//             _viewportHeight - topPadding - bottomPadding
//         );
//         _leftPanel->options = Window::Options {
//             .noTitleBar = false,
//             .noScrollbar = false,
//             .noMenu = true,
//             .noMove = true,
//             .noResize = true,
//             .noCollapse = true,
//             .noNav = true,
//             .noBringToFront = true,
//             .noClose = true
//         };
//     }
//
//     if (_rightPanel && _drawRightPanel)
//     {
//         auto panelSize = getPanelSize(_rightPanelSize, _viewportWidth);
//         _rightPanel->setPosition(_viewportWidth - panelSize, topPadding);
//         _rightPanel->setSize(
//             panelSize,
//             _viewportHeight - topPadding - bottomPadding
//         );
//         _rightPanel->options = Window::Options {
//             .noTitleBar = false,
//             .noScrollbar = false,
//             .noMenu = true,
//             .noMove = true,
//             .noResize = true,
//             .noCollapse = true,
//             .noNav = true,
//             .noBringToFront = true,
//             .noClose = true
//         };
//     }
// }
void Workspace::updateWindows()
{
    uint32_t topPadding = 0;
    uint32_t bottomPadding = 0;
    uint32_t statusPadding = 0;

    //
    // TOOL BAR (arriba del todo)
    //
    if (_toolBar && _drawToolBar)
    {
        topPadding = _toolBarHeight;

        _toolBar->setPosition(0, 0);
        _toolBar->setSize(_viewportWidth, _toolBarHeight);
        _toolBar->options = Window::Options {
            .noTitleBar = true,
            .noScrollbar = true,
            .noMenu = false,
            .noMove = true,
            .noResize = true,
            .noCollapse = true,
            .noNav = true,
            .noBringToFront = true,
            .noClose = true
        };
    }

    //
    // STATUS BAR (abajo del todo)
    //
    if (_statusBar && _drawStatusBar)
    {
        statusPadding = _statusBarHeight;

        _statusBar->setPosition(0, _viewportHeight - statusPadding);
        _statusBar->setSize(_viewportWidth, _statusBarHeight);

        _statusBar->options = Window::Options {
            .noTitleBar = true,
            .noScrollbar = true,
            .noMenu = true,
            .noMove = true,
            .noResize = true,
            .noCollapse = true,
            .noNav = true,
            .noBringToFront = true,
            .noClose = true
        };
    }

    //
    // BOTTOM PANEL (justo encima de la status bar)
    //
    if (_bottomPanel && _drawBottomPanel)
    {
        bottomPadding = getPanelSize(_bottomPanelSize, _viewportHeight);

        _bottomPanel->setPosition(
            0,
            _viewportHeight - statusPadding - bottomPadding
        );
        _bottomPanel->setSize(_viewportWidth, bottomPadding);
        _bottomPanel->options = Window::Options {
            .noTitleBar = false,
            .noScrollbar = false,
            .noMenu = true,
            .noMove = true,
            .noResize = true,
            .noCollapse = true,
            .noNav = true,
            .noBringToFront = true,
            .noClose = true
        };
    }

    //
    // LEFT PANEL
    //
    if (_leftPanel && _drawLeftPanel)
    {
        uint32_t panelWidth = getPanelSize(_leftPanelSize, _viewportWidth);
        uint32_t availableHeight = _viewportHeight - topPadding - bottomPadding - statusPadding;

        _leftPanel->setPosition(0, topPadding);
        _leftPanel->setSize(panelWidth, availableHeight);
        _leftPanel->options = Window::Options {
            .noTitleBar = false,
            .noScrollbar = false,
            .noMenu = true,
            .noMove = true,
            .noResize = true,
            .noCollapse = true,
            .noNav = true,
            .noBringToFront = true,
            .noClose = true
        };
    }

    //
    // RIGHT PANEL
    //
    if (_rightPanel && _drawRightPanel)
    {
        uint32_t panelWidth = getPanelSize(_rightPanelSize, _viewportWidth);
        uint32_t availableHeight = _viewportHeight - topPadding - bottomPadding - statusPadding;

        _rightPanel->setPosition(_viewportWidth - panelWidth, topPadding);
        _rightPanel->setSize(panelWidth, availableHeight);
        _rightPanel->options = Window::Options {
            .noTitleBar = false,
            .noScrollbar = false,
            .noMenu = true,
            .noMove = true,
            .noResize = true,
            .noCollapse = true,
            .noNav = true,
            .noBringToFront = true,
            .noClose = true
        };
    }
}


uint32_t Workspace::getPanelSize(PanelSize & size, uint32_t vpSize)
{
    auto result = static_cast<uint32_t>(static_cast<float>(vpSize) * size.relative);
    if (result > size.max)
    {
        return size.max;
    }
    if (result < size.min)
    {
        return size.min;
    }
    return result;
}

}
