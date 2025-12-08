#pragma once


#include <bg2e.hpp>

class PickSelectionDelegate : public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>,
    public bg2e::app::InputDelegate,
    public bg2e::ui::UserInterfaceDelegate
{
public:
    void init(bg2e::render::Engine * engine) override;
    
    // ============ User Interface Delegate Functions =========
    void init(bg2e::render::Engine*, bg2e::ui::UserInterface*) override;
 
    void swapchainResized(VkExtent2D newSize) override;
    void drawUI() override;
 
    // InputDelegate
    void mouseMove(int x, int y) override;
    void mouseButtonDown(int button, int x, int y) override;
    void mouseButtonUp(int button, int x, int y) override;
    void mouseWheel(int deltaX, int deltaY) override;

    void cleanup() override;

protected:
    bg2e::scene::InputVisitor _inputVisitor;
    bg2e::ui::Workspace _workspace;
    bg2e::ui::Toolbar _toolbar;
    bg2e::ui::Window _leftPanel;
    bg2e::ui::Window _rightPanel;
    bg2e::ui::Window _bottomPanel;
    
    std::shared_ptr<bg2e::manipulation::SelectionManager> _selectionManager;
    
    std::shared_ptr<bg2e::scene::Drawable> _sphere;
    bg2e::scene::EnvironmentComponent * _environment;
    bg2e::scene::OrbitCameraComponent * _orbitCamera;
    
    std::shared_ptr<bg2e::scene::Node> scene1();
    
    std::shared_ptr<bg2e::scene::Node> createScene() override;
};