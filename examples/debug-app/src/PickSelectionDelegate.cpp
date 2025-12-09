
#include <PickSelectionDelegate.hpp>

void PickSelectionDelegate::init(bg2e::render::Engine * engine)
{
    bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>::init(engine);
    
    _selectionManager = std::make_shared<bg2e::manipulation::SelectionManager>(engine);
    _selectionManager->init();
}
    
void PickSelectionDelegate::init(bg2e::render::Engine*, bg2e::ui::UserInterface*)
{
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


    _toolbar.addButton({
        .label = "Screenshot",
        .action = [&]() {
            bg2e::app::FileDialog fd;
            fd.setFilters({
                { "Images", "jpg,jpeg,png,bmp,tga" }
            });
            auto filePath = fd.saveFile();

            if (!filePath.empty())
            {
                std::vector<uint8_t> outData;
                uint32_t width, height, bpp;
                _engine->swapchain().screenshot(outData, width, height, bpp);

                bg2e::db::saveImage(
                    filePath,
                    outData.data(),
                    width,
                    height,
                    bpp
                );
            }

        }
    });
    
    
    _leftPanel.setDrawFunction([&]() {
    
        auto &material = _sphere->material(0);
        float metalness = material.metalness();
        float roughness = material.roughness();
        bg2e::base::Color albedo = material.albedo();
        bg2e::ui::BasicWidgets::separator();
        bg2e::ui::Input::slider("Metalness", &metalness, 0.0f, 1.0f);
        bg2e::ui::Input::slider("Roughness", &roughness, 0.0f, 1.0f);
        bg2e::ui::Input::colorPicker("Albedo", albedo);
        material.setMetalness(metalness);
        material.setRoughness(roughness);
        material.setAlbedo(albedo);
        _sphere->updateMaterials();

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
        float exposure = renderer()->exposure();
        bg2e::ui::Input::slider("Brightness", &brightness, 0.0f, 1.0f);
        bg2e::ui::Input::slider("Contrast", &contrast, 0.0f, 2.0f);
        bg2e::ui::Input::slider("Exposure", &exposure, 0.0f, 5.0f);
        renderer()->setBrightness(brightness);
        renderer()->setContrast(contrast);
        renderer()->setExposure(exposure);
    });
}
 
void PickSelectionDelegate::swapchainResized(VkExtent2D newSize)
{
    DefaultRenderLoopDelegate::swapchainResized(newSize);
    _workspace.resize(uiWidth(), uiHeight());
}

void PickSelectionDelegate::drawUI()
{
    _workspace.draw();
}

// InputDelegate
void PickSelectionDelegate::mouseMove(int x, int y)
{
    _inputVisitor.mouseMove(renderer()->scene()->rootNode(), x, y);
}

void PickSelectionDelegate::mouseButtonDown(int button, int x, int y)
{
    _inputVisitor.mouseButtonDown(renderer()->scene()->rootNode(), button, x, y);
}

void PickSelectionDelegate::mouseButtonUp(int button, int x, int y)
{
    _inputVisitor.mouseButtonUp(renderer()->scene()->rootNode(), button, x, y);

    if (_selectionManager->pick(
        renderer()->scene(),
        x,
        y
    )) {
        auto mesh = _selectionManager->selectedMesh();
        auto submesh = _selectionManager->selectedSubmesh();
        if (mesh)
        {
            std::cout << "Selected item: " << mesh->submeshName(submesh) << std::endl;
        }
    }
}

void PickSelectionDelegate::mouseWheel(int deltaX, int deltaY)
{
    _inputVisitor.mouseWheel(renderer()->scene()->rootNode(), deltaX, deltaY);
}

void PickSelectionDelegate::cleanup()
{
    bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>::cleanup();
}

std::shared_ptr<bg2e::scene::Node> PickSelectionDelegate::scene1()
{
    auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
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
    
    
    // Procedural geometry
    
    glm::vec3 pos{0};
    const uint32_t rows = 10;
    const uint32_t cols = 10;
    float sphereRadius = 1.0f;
    float sphereGap = 0.5f;
    float sphereSeparation = sphereRadius * 2.0f + sphereGap;
    float totalWidth = static_cast<float>(rows) * sphereSeparation;
    float totalHeight = static_cast<float>(cols) * sphereSeparation;
    auto sphereMesh = std::shared_ptr<bg2e::geo::Mesh>(bg2e::geo::createSphere(sphereRadius, 35, 35));
    bg2e::geo::GenTangentsModifier<bg2e::geo::MeshPNUUT> genTangents(sphereMesh.get());
    genTangents.apply();
    for (uint32_t row = 0; row < rows; ++row)
    {
        for (uint32_t col = 0; col < cols; ++col)
        {
            pos.x = (totalWidth / -2.0f) + sphereSeparation / 2.0f + static_cast<float>(row) * sphereSeparation;
            pos.y = (totalHeight / -2.0f) + sphereSeparation / 2.0f + static_cast<float>(col) * sphereSeparation;
            
            auto sphereDrawable = std::make_shared<bg2e::scene::Drawable>();
            sphereDrawable->setMesh(sphereMesh);
            sphereDrawable->material(0).setMetalness(static_cast<float>(row) / rows);
            sphereDrawable->material(0).setRoughness(static_cast<float>(col) / cols);
            
            if (col % 2 == 0 && row % 2 == 0)
            {
                sphereDrawable->material(0).setIsTransparent(true);
                sphereDrawable->material(0).setAlbedo(bg2e::base::Color(1.0f, 0.0f, 0.0f, 0.7f));
            }
            else
            {
                sphereDrawable->material(0).setAlbedo(bg2e::base::Color::Red());
            }
            
            sphereDrawable->load(_engine);
            sphereDrawable->setSubmeshName("Sphere-" + std::to_string(row) + "_" + std::to_string(col) + "-submesh");
            auto node = std::make_shared<bg2e::scene::Node>("Sphere-" + std::to_string(row) + "-" + std::to_string(col));
            node->addComponent(new bg2e::scene::DrawableComponent(sphereDrawable));
            node->addComponent(bg2e::scene::TransformComponent::makeTranslated(pos));
            node->addComponent(new bg2e::manipulation::SelectableComponent());
            sceneRoot->addChild(node);
        }
    }
    
    
    auto mainSphereDrawable = std::make_shared<bg2e::scene::Drawable>();
    mainSphereDrawable->setMesh(sphereMesh);
    mainSphereDrawable->material(0).setAlbedoTexture(new bg2e::base::Texture(assetPath, "rust_metal_albedo.jpg"));
    mainSphereDrawable->material(0).setNormalTexture(new bg2e::base::Texture(assetPath, "rust_metal_normal.jpg"));
    mainSphereDrawable->material(0).setMetalnessTexture(new bg2e::base::Texture(assetPath, "rust_metal_metallic.jpg"));
    mainSphereDrawable->material(0).setRoughnessTexture(new bg2e::base::Texture(assetPath, "rust_metal_roughness.jpg"));
    mainSphereDrawable->load(_engine);
    
    auto mainSphereNode = new bg2e::scene::Node("Main Sphere");
    mainSphereNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(-5.0f, 0.0f, 4.0f));
    mainSphereNode->transform()->scale(3.0f);
    mainSphereNode->addComponent(new bg2e::scene::DrawableComponent(mainSphereDrawable));
    sceneRoot->addChild(mainSphereNode);
    
    auto customSphereDrawable = std::make_shared<bg2e::scene::Drawable>();
    customSphereDrawable->setMesh(sphereMesh);
    customSphereDrawable->material(0).setMetalness(0.6f);
    customSphereDrawable->material(0).setRoughness(0.2f);
    customSphereDrawable->load(_engine);
    auto sphere = customSphereDrawable;
    auto customSphereNode = new bg2e::scene::Node("Custom Sphere");
    customSphereNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(5.0f, 0.0f, 4.0f));
    customSphereNode->transform()->scale(3.0f);
    customSphereNode->addComponent(new bg2e::scene::DrawableComponent(customSphereDrawable));
    _sphere = customSphereDrawable;
    
    
    sceneRoot->addChild(customSphereNode);
    
    
    _engine->cleanupManager().push([&](VkDevice) {
        _sphere.reset();
    });
    
    return sceneRoot;
}

std::shared_ptr<bg2e::scene::Node> PickSelectionDelegate::createScene()
{
    return scene1();
}
