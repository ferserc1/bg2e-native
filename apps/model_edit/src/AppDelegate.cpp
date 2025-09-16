//
//  AppDelegate.cpp
//  model_edit
//
//  Created by Fernando Serrano Carpena on 16/9/25.
//

#include "AppDelegate.hpp"


class CameraMouse : public bg2e::scene::Component {
public:
    BG2E_COMPONENT_TYPE_NAME("CameraMouse")
    
    CameraMouse(bg2e::scene::Node * cameraNode) :_cameraNode(cameraNode) {}
    
    void update(float delta) override
    {
        auto transform = ownerNode()->transform();
        
        if (transform)
        {
            auto rv = _cameraNode->rightVector();
            auto uv = _cameraNode->upVector();

            transform->translate(
                rv * glm::vec3{ _pan.x * 0.01, 0, 0 } +
                uv * glm::vec3{ 0, -_pan.y * 0.01, 0 }
            );
            
            transform->rotate(_rot.y * 0.01f, glm::vec3(1, 0, 0));
            transform->rotate(_rot.x * 0.01f, glm::vec3(0, 1, 0));
            
            _rot.x = 0.0f;
            _rot.y = 0.0f;
            _pan = glm::vec3 { 0.0f };
        }
    }
    
    void mouseMove(int x, int y) override
    {
        if (_action == ActionOrbit)
        {
            _rot.x = static_cast<float>(_x - x);
            _rot.y = static_cast<float>(_y - y);
            _x = x;
            _y = y;
        }
        else if (_action == ActionPan)
        {
            _pan = glm::vec2(static_cast<float>(_x - x), static_cast<float>(_y - y));
            _x = x;
            _y = y;
        }
    }
    
    void mouseButtonDown(int button, int x, int y) override
    {
        _action = button == 0 ? ActionOrbit : ActionPan;
        _x = x;
        _y = y;
    }
    
    void mouseButtonUp(int button, int, int) override
    {
        _action = ActionNone;
    }
    
    void mouseWheel(int deltaX, int deltaY) override
    {
        auto camera = _cameraNode ? _cameraNode->camera() : nullptr;
        auto opticalProjection = camera ? dynamic_cast<bg2e::math::OpticalProjection*>(camera->projection()) : nullptr;
        
        if (opticalProjection)
        {
            auto fl = opticalProjection->focalLength() + static_cast<float>(deltaY) * 0.5f;
            fl = fl < _minFocalLength ? _minFocalLength : fl;
            fl = fl > _maxFocalLength ? _maxFocalLength : fl;
            opticalProjection->setFocalLength(fl);
        }
    }
    
protected:
    bg2e::scene::Node * _cameraNode = nullptr;
    enum Action {
        ActionNone = 0,
        ActionOrbit = 1,
        ActionPan = 2
    };
    Action _action = ActionNone;
    int _x = 0;
    int _y = 0;
    glm::vec2 _rot { 0, 0 };
    glm::vec2 _pan { 0, 0 };
    float _minFocalLength = 18.0f;
    float _maxFocalLength = 300.0f;
};


void AppDelegate::init(bg2e::render::Engine * engine)
{
    bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>::init(engine);
}

// ============ User Interface Delegate Functions =========
void AppDelegate::init(bg2e::render::Engine*, bg2e::ui::UserInterface*)
{
    _window.setTitle("Basic forward renderer");
    _window.options.noClose = true;
    _window.options.minWidth = 300;
    _window.options.minHeight = 190;
    _window.setPosition(0, 50);
    _window.setSize(300, 290);

    _environmentSettings = std::make_unique<EnvironmentSettings>();
    _environmentSettings->init(uiWidth(), uiHeight());
    
    _menuBar = std::make_unique<ToolBar>();
    _menuBar->init(uiWidth(), _environmentSettings);
}

void AppDelegate::swapchainResized(VkExtent2D extent)
{
    DefaultRenderLoopDelegate::swapchainResized(extent);
    _menuBar->resize(uiWidth());
    _environmentSettings->resizeViewport(uiWidth(), uiHeight());
}

void AppDelegate::drawUI()
{
    _menuBar->draw();
    
    _environmentSettings->draw(renderer(), _environment);
}

// InputDelegate
void AppDelegate::mouseMove(int x, int y)
{
    _inputVisitor.mouseMove(renderer()->scene()->rootNode(), x, y);
}

void AppDelegate::mouseButtonDown(int button, int x, int y)
{
    _inputVisitor.mouseButtonDown(renderer()->scene()->rootNode(), button, x, y);
}

void AppDelegate::mouseButtonUp(int button, int x, int y)
{
    _inputVisitor.mouseButtonUp(renderer()->scene()->rootNode(), button, x, y);
}

void AppDelegate::mouseWheel(int deltaX, int deltaY)
{
    _inputVisitor.mouseWheel(renderer()->scene()->rootNode(), deltaX, deltaY);
}

std::shared_ptr<bg2e::scene::Node> AppDelegate::scene1()
{
    auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
    //sceneRoot->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "equirectangular-env3.jpg"));
    sceneRoot->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "gothic_manor_01_4k.hdr"));
    _environment = sceneRoot->environment();
    
    
    
    auto cameraNode = std::shared_ptr<bg2e::scene::Node>(new bg2e::scene::Node("Camera"));
    cameraNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 0.5f, 2.0f ));
    
    cameraNode->addComponent(new bg2e::scene::CameraComponent());
    auto projection = new bg2e::math::OpticalProjection();
    cameraNode->camera()->setProjection(projection);
    
    auto cameraRotation = new bg2e::scene::Node("Camera Rotation");
    cameraRotation->addComponent(new bg2e::scene::TransformComponent());
    cameraRotation->addComponent(new CameraMouse(cameraNode.get()));
    cameraRotation->addChild(cameraNode);
    sceneRoot->addChild(cameraRotation);
    
    auto light1 = new bg2e::scene::Node("Light 1");
    light1->addComponent(new bg2e::scene::LightComponent());
    light1->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4 { 1.0f }, glm::vec3{-5, 5, 5 } )));
    light1->light()->light().setIntensity(300.0f);
    sceneRoot->addChild(light1);
    
    auto armchairData = std::shared_ptr<bg2e::db::Bg2Mesh>(bg2e::db::loadMeshBg2(bg2e::base::PlatformTools::assetPath(), "armchair.bg2"));
    auto armchairNode = new bg2e::scene::Node("Armchair");
    auto drawable = std::make_shared<bg2e::scene::Drawable>();
    drawable->setMesh(armchairData->mesh);
    uint32_t submesh = 0;
    for (auto & m : armchairData->materials)
    {
        drawable->setMaterial(m, submesh);
        submesh++;
    }
    drawable->load(_engine);
    drawable->updateMaterials();
    armchairNode->addComponent(new bg2e::scene::DrawableComponent(drawable));
    sceneRoot->addChild(armchairNode);
    
   
    return sceneRoot;
}

std::shared_ptr<bg2e::scene::Node> AppDelegate::createScene()
{
    return scene1();
}
