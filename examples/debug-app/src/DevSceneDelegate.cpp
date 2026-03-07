
#include <DevSceneDelegate.hpp>

void DevSceneDelegate::init(bg2e::render::Engine * engine)
{
    bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>::init(engine);
    
    _selectionManager = std::make_shared<bg2e::manipulation::SelectionManager>(engine);
    _selectionManager->init();

    _drawableEditor.init(_selectionManager);
    _materialEditor.setSelectionManager(_selectionManager);
}
    
void DevSceneDelegate::init(bg2e::render::Engine*, bg2e::ui::UserInterface*)
{

    bg2e::app::MainLoop::current()->setOnExitFunction([]() -> bool {
        auto status = bg2e::app::MessageBox::showWarning(
            "Quit", "Are you sure you want to quit?",
            {
                { .code = 1, .label = "Quit" },
                { .code = 2,  .label = "Cancel", .key = bg2e::app::MessageBox::Esc }
            }
        );
        return status == 1;
    });

    _leftPanel.setTitle("Model & Submeshes");
    _rightPanel.setTitle("File");
    _bottomPanel.setTitle("Environment");
    
    _workspace.leftPanelSize().min = 300;
    _workspace.rightPanelSize().min = 250;
    
    _workspace.setup(
        uiWidth(),
        uiHeight(),
        &_toolbar,
        &_leftPanel,
        &_rightPanel,
        &_bottomPanel
    );
    
    _toolbar.setMenuFunction([&]() {
        if (bg2e::ui::Menu::beginMenu("File"))
        {
            if (bg2e::ui::Menu::menuItem("Open"))
            {
                bg2e::app::FileDialog fd;
                fd.setFilters({
                    { "bg2e 3D model", "bg2,vwglb" }
                });
                auto filePath = fd.openFile();
                
                if (!filePath.empty())
                {
                    auto modelDrawable = bg2e::db::loadDrawableBg2(filePath, _engine);
                    _targetDrawable = modelDrawable;
                    auto modelNode = new bg2e::scene::Node("Armchair");
                    modelNode->addComponent(new bg2e::scene::DrawableComponent(modelDrawable));
                    modelNode->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4{ 1.0 }, glm::vec3{ 5, 0, 0 })));
                    modelNode->transform()->scale(4.0f);
                    renderer()->scene()->rootNode()->addChild(modelNode);
                }
            }
            if (bg2e::ui::Menu::menuItem("Save Object"))
            {
                bg2e::app::FileDialog fd;
                fd.setFilters({
                    { "bg2e 3D model", "bg2" }
                });
                auto filePath = fd.saveFile();
                
                if (!filePath.empty())
                {
//                        bg2e::db::saveScene(
//                            renderer()->scene()->rootNode(),
//                            filePath
//                        );
                    bg2e::db::storeDrawableBg2(
                        filePath,
                        _targetDrawable
                    );
                }
            }
            bg2e::ui::Menu::separator();
            if (bg2e::ui::Menu::menuItem("Quit"))
            {
                bg2e::app::MainLoop::current()->exit();
            }
            bg2e::ui::Menu::endMenu();
        }
        
        if (bg2e::ui::Menu::beginMenu("View"))
        {
            if (bg2e::ui::Menu::menuItem("Center View", "Ctrl+A"))
            {
                _orbitCamera->reset();
            }
            bg2e::ui::Menu::endMenu();
        }
        
    });
    
    
    _leftPanel.setDrawFunction([&]() {
    
//            auto &material = _sphere->material(0);
//            float metalness = material.metalness();
//            float roughness = material.roughness();
//            bg2e::base::Color albedo = material.albedo();
//            bg2e::ui::BasicWidgets::separator();
//            bg2e::ui::Input::slider("Metalness", &metalness, 0.0f, 1.0f);
//            bg2e::ui::Input::slider("Roughness", &roughness, 0.0f, 1.0f);
//            bg2e::ui::Input::colorPicker("Albedo", albedo);
//            material.setMetalness(metalness);
//            material.setRoughness(roughness);
//            material.setAlbedo(albedo);
//            _sphere->material(1).setMetalness(metalness);
//            _sphere->material(1).setRoughness(roughness);
//            _sphere->material(2).setMetalness(metalness);
//            _sphere->material(2).setRoughness(roughness);
//            _sphere->material(3).setMetalness(metalness);
//            _sphere->material(3).setRoughness(roughness);
//            _sphere->material(4).setMetalness(metalness);
//            _sphere->material(4).setRoughness(roughness);
//            _sphere->material(5).setMetalness(metalness);
//            _sphere->material(5).setRoughness(roughness);
//            _sphere->updateMaterials();

        auto name = _targetDrawable->name();
        if (bg2e::ui::Input::text("Drawable Name", name))
        {
            _targetDrawable->setName(name);
        }

        _drawableEditor.draw();
        _materialEditor.draw();
    });
    
    _bottomPanel.setDrawFunction([&]() {
        auto drawSkybox = renderer()->drawSkybox();
        auto blurLevel = renderer()->skyboxBlurLevel();
        bg2e::ui::BasicWidgets::checkBox("Draw Skybox", &drawSkybox);
        if (_environment)
        {
            auto assetPath = bg2e::base::PlatformTools::assetPath();
            if (bg2e::ui::BasicWidgets::button("Mirrored Hall"))
            {
                _environment->setEnvironmentImage(assetPath, "mirrored_hall_4k.hdr");
            }
            if (bg2e::ui::BasicWidgets::button("Theater", true))
            {
                _environment->setEnvironmentImage(assetPath, "theater_01_4k.hdr");
            }
            if (bg2e::ui::BasicWidgets::button("Autum Field", true))
            {
                _environment->setEnvironmentImage(assetPath, "autumn_field_4k.hdr");
            }
            if (bg2e::ui::BasicWidgets::button("Gothic Manor", true))
            {
                _environment->setEnvironmentImage(assetPath, "gothic_manor_01_4k.hdr");
            }
            if (bg2e::ui::BasicWidgets::button("Black Environment", true))
            {
                _environment->setEnvironmentImage(assetPath, "black.jpg");
            }
            if (bg2e::ui::Input::slider("Skbox Blur Level", &blurLevel, 0, 5)) {
                renderer()->setSkyboxBlurLevel(blurLevel);
            }
        }
    
        renderer()->setDrawSkybox(drawSkybox);
    });
    
    _rightPanel.setDrawFunction([&]() {
        float brightness = renderer()->brightness();
        float contrast = renderer()->contrast();
        bg2e::ui::Input::slider("Brightness", &brightness, 0.0f, 1.0f);
        bg2e::ui::Input::slider("Contrast", &contrast, 0.0f, 2.0f);
        renderer()->setBrightness(brightness);
        renderer()->setContrast(contrast);
    });
    
    
    int32_t testButtonId = _toolbar.addButton({
        .label = "Test Button",
        .action = []() { std::cout << "Hello World" << std::endl; }
    });
    
    _toolbar.addButton({
        .label = "Other test",
        .action = []() { std::cout << "Hello again" << std::endl; }
    }, bg2e::ui::Toolbar::AlignRight);
    
    _toolbar.addButton({
        .label = "This is a text"
    }, bg2e::ui::Toolbar::AlignRight);
    
    static int32_t value = 0;
    auto textId = _toolbar.addButton({
        .label = "Count = " + std::to_string(value)
    }, bg2e::ui::Toolbar::AlignRight);
    
    _toolbar.addButton({
        .label = "Right Button",
        .action = [&, textId]() {
            ++value;
            _toolbar.updateButtonLabel(textId, "Count = " + std::to_string(value));
        }
    }, bg2e::ui::Toolbar::AlignRight);
    
    static bool buttonEnabled = true;
    _toolbar.addButton({
        .label = "Test Enabled",
        .action = [&,testButtonId]() {
            if (buttonEnabled)
            {
                _toolbar.disableButton(testButtonId);
            }
            else
            {
                _toolbar.enableButton(testButtonId);
            }
            buttonEnabled = !buttonEnabled;
        }
    });
    
//        _toolbar.setDrawFunction([&]() {
//            using namespace bg2e::ui;
//            static uint32_t clicks = 0;
//            if (BasicWidgets::button("Test button"))
//            {
//                ++clicks;
//            }
//            BasicWidgets::text("Button clicked " + std::to_string(clicks) + " times", true);
//            
//            auto leftPanel = _workspace.leftPanelVisible();
//            auto rightPanel = _workspace.rightPanelVisible();
//            auto bottomPanel = _workspace.bottomPanelVisible();
//            
//            auto leftText = leftPanel ? "Hide Model Panel" : "Show Model Panel";
//            auto rightText = rightPanel ? "Hide File Panel" : "Show File Panel";
//            auto bottomText = bottomPanel ? "Hide Environment Panel" : "Show Environment Panel";
//            auto buttonsSize = BasicWidgets::calcButtonWidth(leftText) +
//                BasicWidgets::calcButtonWidth(rightText) +
//                BasicWidgets::calcButtonWidth(bottomText) +
//                BasicWidgets::getItemHorizontalSpacing() * 3;
//            BasicWidgets::sameLine(-buttonsSize);
//            if (BasicWidgets::button(leftText))
//            {
//                _workspace.toggleLeftPanel();
//            }
//            
//            if (BasicWidgets::button(rightText, true))
//            {
//                _workspace.toggleRightPanel();
//            }
//            
//            if (BasicWidgets::button(bottomText, true))
//            {
//                _workspace.toggleBottomPanel();
//            }
//        });
}
 
void DevSceneDelegate::swapchainResized(VkExtent2D newSize)
{
    DefaultRenderLoopDelegate::swapchainResized(newSize);
    _workspace.resize(uiWidth(), uiHeight());
}

void DevSceneDelegate::drawUI()
{
    _workspace.draw();
}

// InputDelegate
void DevSceneDelegate::mouseMove(int x, int y)
{
    _inputVisitor.mouseMove(renderer()->scene()->rootNode(), x, y);
}

void DevSceneDelegate::mouseButtonDown(int button, int x, int y)
{
    _inputVisitor.mouseButtonDown(renderer()->scene()->rootNode(), button, x, y);
}

void DevSceneDelegate::mouseButtonUp(int button, int x, int y)
{
    _inputVisitor.mouseButtonUp(renderer()->scene()->rootNode(), button, x, y);
    _selectionManager->pick(this->renderer()->scene(), x, y);
}

void DevSceneDelegate::mouseWheel(int deltaX, int deltaY)
{
    _inputVisitor.mouseWheel(renderer()->scene()->rootNode(), deltaX, deltaY);
}

void DevSceneDelegate::cleanup()
{
    bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>::cleanup();
    _materialEditor.cleanup();
    _drawableEditor.cleanup();
}

std::shared_ptr<bg2e::scene::Node> DevSceneDelegate::scene1()
{
    auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
    //sceneRoot->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "equirectangular-env3.jpg"));
    sceneRoot->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "gothic_manor_01_4k.hdr"));
    _environment = sceneRoot->environment();
    
    
    
    auto cameraNode = std::shared_ptr<bg2e::scene::Node>(new bg2e::scene::Node("Camera"));
    cameraNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 1.6f, 5.0f ));
    
    cameraNode->addComponent(new bg2e::scene::CameraComponent());
    auto projection = new bg2e::math::OpticalProjection();
    projection->setFar(1000.0f);
    cameraNode->camera()->setProjection(projection);
    
    auto cameraRotation = new bg2e::scene::Node("Camera Rotation");
    cameraRotation->addComponent(new bg2e::scene::TransformComponent());

    _orbitCamera = new bg2e::scene::OrbitCameraComponent();
    _orbitCamera->setMaxDistance(100.0f);
    cameraRotation->addComponent(_orbitCamera);
    cameraRotation->addChild(cameraNode);
    sceneRoot->addChild(cameraRotation);
    
    auto assetPath = bg2e::base::PlatformTools::assetPath();
        
    auto model = bg2e::db::loadDrawableBg2(assetPath, "armchair.bg2", _engine);
    auto modelNode = new bg2e::scene::Node("Armchair");
    _targetDrawable = std::shared_ptr<bg2e::scene::Drawable>(model);

    modelNode->addComponent(new bg2e::manipulation::SelectableComponent());
    modelNode->addComponent(new bg2e::scene::DrawableComponent(model));
    modelNode->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4 { 1.0 }, glm::vec3{ 0, 0, 0 })));
    modelNode->transform()->scale(4.0f);
    sceneRoot->addChild(modelNode);
    
    
    _engine->cleanupManager().push([&](VkDevice) {
        _targetDrawable.reset();
    });
    
    return sceneRoot;
}

std::shared_ptr<bg2e::scene::Node> DevSceneDelegate::createScene()
{
    return scene1();
}
