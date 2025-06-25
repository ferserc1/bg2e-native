//
//#include <bg2e.hpp>
//#include <numbers>
//
//class RotateCameraComponent : public bg2e::scene::Component {
//public:
//    RotateCameraComponent() :_r{0.001f} {}
//    RotateCameraComponent(float r) :_r{r} {}
//    
//    void update(float delta) override
//    {
//        auto transform = ownerNode()->transform();
//        
//        if (transform)
//        {
//            transform->rotate(_r * delta / 10.0f, 0.0f, 1.0f, 0.0f);
//        }
//    }
//    
//protected:
//    float _r;
//};
//
//class CameraMouse : public bg2e::scene::Component {
//public:
//    CameraMouse(bg2e::scene::Node * cameraNode) :_cameraNode(cameraNode) {}
//    
//    void update(float delta) override
//    {
//        auto transform = ownerNode()->transform();
//        
//        if (transform)
//        {
//            auto rv = _cameraNode->rightVector();
//            auto uv = _cameraNode->upVector();
//
//            transform->translate(
//                rv * glm::vec3{ _pan.x * 0.1, 0, 0 } +
//                uv * glm::vec3{ 0, -_pan.y * 0.1, 0 }
//            );
//            
//            transform->rotate(_rot.y * 0.01f, glm::vec3(1, 0, 0));
//            transform->rotate(_rot.x * 0.01f, glm::vec3(0, 1, 0));
//            
//            _rot.x = 0.0f;
//            _rot.y = 0.0f;
//            _pan = glm::vec3 { 0.0f };
//        }
//    }
//    
//    void mouseMove(int x, int y) override
//    {
//        if (_action == ActionOrbit)
//        {
//            _rot.x = static_cast<float>(_x - x);
//            _rot.y = static_cast<float>(_y - y);
//            _x = x;
//            _y = y;
//        }
//        else if (_action == ActionPan)
//        {
//            _pan = glm::vec2(static_cast<float>(_x - x), static_cast<float>(_y - y));
//            _x = x;
//            _y = y;
//        }
//    }
//    
//    void mouseButtonDown(int button, int x, int y) override
//    {
//        _action = button == 0 ? ActionOrbit : ActionPan;
//        _x = x;
//        _y = y;
//    }
//    
//    void mouseButtonUp(int button, int, int) override
//    {
//        _action = ActionNone;
//    }
//    
//    void mouseWheel(int deltaX, int deltaY) override
//    {
//        auto camera = _cameraNode ? _cameraNode->camera() : nullptr;
//        auto opticalProjection = camera ? dynamic_cast<bg2e::math::OpticalProjection*>(camera->projection()) : nullptr;
//        
//        if (opticalProjection)
//        {
//            auto fl = opticalProjection->focalLength() + static_cast<float>(deltaY) * 0.5f;
//            fl = fl < _minFocalLength ? _minFocalLength : fl;
//            fl = fl > _maxFocalLength ? _maxFocalLength : fl;
//            opticalProjection->setFocalLength(fl);
//        }
//    }
//    
//protected:
//    bg2e::scene::Node * _cameraNode = nullptr;
//    enum Action {
//        ActionNone = 0,
//        ActionOrbit = 1,
//        ActionPan = 2
//    };
//    Action _action = ActionNone;
//    int _x = 0;
//    int _y = 0;
//    glm::vec2 _rot { 0, 0 };
//    glm::vec2 _pan { 0, 0 };
//    float _minFocalLength = 18.0f;
//    float _maxFocalLength = 300.0f;
//};
//
//class BasicSceneDelegate : public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>,
//	public bg2e::app::InputDelegate,
//	public bg2e::ui::UserInterfaceDelegate
//{
//public:
//    void init(bg2e::render::Engine * engine) override
//    {
//        bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>::init(engine);
//    }
//    
//	// ============ User Interface Delegate Functions =========
//	void init(bg2e::render::Engine*, bg2e::ui::UserInterface*) override {
//		_window.setTitle("Basic forward renderer");
//		_window.options.noClose = true;
//		_window.options.minWidth = 300;
//		_window.options.minHeight = 190;
//		_window.setPosition(0, 0);
//		_window.setSize(300, 290);
//	}
//
//	void drawUI() override
//	{
//        auto drawSkybox = renderer()->drawSkybox();
//    
//        _window.draw([&]() {
//            bg2e::ui::BasicWidgets::checkBox("Draw Skybox", &drawSkybox);
//        
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
//            _sphere->updateMaterials();
//            
//            if (_environment)
//            {
//                auto assetPath = bg2e::base::PlatformTools::assetPath();
//                if (bg2e::ui::BasicWidgets::button("Mirrored Hall"))
//                {
//                    _environment->setEnvironmentImage(assetPath, "mirrored_hall_4k.hdr");
//                }
//                if (bg2e::ui::BasicWidgets::button("Theater"))
//                {
//                    _environment->setEnvironmentImage(assetPath, "theater_01_4k.hdr");
//                }
//                if (bg2e::ui::BasicWidgets::button("Autum Field"))
//                {
//                    _environment->setEnvironmentImage(assetPath, "autumn_field_4k.hdr");
//                }
//                if (bg2e::ui::BasicWidgets::button("Gothic Manor"))
//                {
//                    _environment->setEnvironmentImage(assetPath, "gothic_manor_01_4k.hdr");
//                }
//            }
//        });
//        renderer()->setDrawSkybox(drawSkybox);
//	}
// 
//    // InputDelegate
//    void mouseMove(int x, int y) override
//    {
//        _inputVisitor.mouseMove(renderer()->scene()->rootNode(), x, y);
//    }
//    
//    void mouseButtonDown(int button, int x, int y) override
//    {
//        _inputVisitor.mouseButtonDown(renderer()->scene()->rootNode(), button, x, y);
//    }
//    
//    void mouseButtonUp(int button, int x, int y) override
//    {
//        _inputVisitor.mouseButtonUp(renderer()->scene()->rootNode(), button, x, y);
//    }
//    
//    void mouseWheel(int deltaX, int deltaY) override
//    {
//        _inputVisitor.mouseWheel(renderer()->scene()->rootNode(), deltaX, deltaY);
//    }
//
//protected:
//	bg2e::ui::Window _window;
//    bg2e::scene::InputVisitor _inputVisitor;
//    
//    std::shared_ptr<bg2e::scene::Drawable> _sphere;
//    bg2e::scene::EnvironmentComponent * _environment;
//    
//    std::shared_ptr<bg2e::scene::Node> scene1()
//    {
//        auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
//        //sceneRoot->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "equirectangular-env3.jpg"));
//        sceneRoot->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "gothic_manor_01_4k.hdr"));
//        _environment = sceneRoot->environment();
//        
//        
//        
//        auto cameraNode = std::shared_ptr<bg2e::scene::Node>(new bg2e::scene::Node("Camera"));
//        cameraNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 0.0f, 45.0f ));
//        
//        cameraNode->addComponent(new bg2e::scene::CameraComponent());
//        auto projection = new bg2e::math::OpticalProjection();
//        cameraNode->camera()->setProjection(projection);
//        
//        auto cameraRotation = new bg2e::scene::Node("Camera Rotation");
//        cameraRotation->addComponent(new bg2e::scene::TransformComponent());
//        //cameraRotation->addComponent(new RotateCameraComponent(-0.002f));
//        cameraRotation->addComponent(new CameraMouse(cameraNode.get()));
//        cameraRotation->addChild(cameraNode);
//        sceneRoot->addChild(cameraRotation);
//        
//        // Lights: When you first create the scene in createScene(), you don't need to do anything with the lights, because the
//        // scene has not been initialised yet. Once the scene is initialised, if lights are added or removed during the rendering
//        // loop, _scene->updateLights() must be called to rebuild the array of lights in the scene that will be passed to the shader.
//        auto light1 = new bg2e::scene::Node("Light 1");
//        light1->addComponent(new bg2e::scene::LightComponent());
//        light1->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4 { 1.0f }, glm::vec3{-10, 10, 10 } )));
//        light1->light()->light().setIntensity(300.0f);
//        sceneRoot->addChild(light1);
//        
//        auto light2 = new bg2e::scene::Node("Light 2");
//        light2->addComponent(new bg2e::scene::LightComponent());
//        light2->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4 { 1.0f }, glm::vec3{ 10, 10, 10 } )));
//        light2->light()->light().setIntensity(300.0f);
//        sceneRoot->addChild(light2);
//        
//        auto light3 = new bg2e::scene::Node("Light 3");
//        light3->addComponent(new bg2e::scene::LightComponent());
//        light3->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4 { 1.0f }, glm::vec3{-10,-10, 10 } )));
//        light3->light()->light().setIntensity(300.0f);
//        sceneRoot->addChild(light3);
//        
//        auto light4 = new bg2e::scene::Node("Light 4");
//        light4->addComponent(new bg2e::scene::LightComponent());
//        light4->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4 { 1.0f }, glm::vec3{ 10,-10, 10 } )));
//        light4->light()->light().setIntensity(300.0f);
//        sceneRoot->addChild(light4);
//        
//        // Procedural geometry
//        glm::vec3 pos{0};
//        const uint32_t rows = 10;
//        const uint32_t cols = 10;
//        float sphereRadius = 1.0f;
//        float sphereGap = 0.5f;
//        float sphereSeparation = sphereRadius * 2.0f + sphereGap;
//        float totalWidth = static_cast<float>(rows) * sphereSeparation;
//        float totalHeight = static_cast<float>(cols) * sphereSeparation;
//        auto sphereMesh = std::shared_ptr<bg2e::geo::Mesh>(bg2e::geo::createSphere(sphereRadius, 15, 15));
//        bg2e::geo::GenTangentsModifier<bg2e::geo::MeshPNUUT> genTangents(sphereMesh.get());
//        genTangents.apply();
//        for (uint32_t row = 0; row < rows; ++row)
//        {
//            for (uint32_t col = 0; col < cols; ++col)
//            {
//                pos.x = (totalWidth / -2.0f) + sphereSeparation / 2.0f + static_cast<float>(row) * sphereSeparation;
//                pos.y = (totalHeight / -2.0f) + sphereSeparation / 2.0f + static_cast<float>(col) * sphereSeparation;
//                
//                auto sphereDrawable = std::make_shared<bg2e::scene::Drawable>();
//                sphereDrawable->setMesh(sphereMesh);
//                sphereDrawable->material(0).setMetalness(static_cast<float>(row) / rows);
//                sphereDrawable->material(0).setRoughness(static_cast<float>(col) / cols);
//                sphereDrawable->material(0).setAlbedo(bg2e::base::Color::Red());
//                
//                sphereDrawable->load(_engine);
//                auto node = std::make_shared<bg2e::scene::Node>("Sphere-" + std::to_string(row) + "-" + std::to_string(col));
//                node->addComponent(new bg2e::scene::DrawableComponent(sphereDrawable));
//                node->addComponent(bg2e::scene::TransformComponent::makeTranslated(pos));
//                sceneRoot->addChild(node);
//            }
//        }
//        
//        auto assetPath = bg2e::base::PlatformTools::assetPath();
//        auto mainSphereDrawable = new bg2e::scene::Drawable();
//        mainSphereDrawable->setMesh(sphereMesh);
//        mainSphereDrawable->material(0).setAlbedo(new bg2e::base::Texture(assetPath, "rust_metal_albedo.jpg"));
//        mainSphereDrawable->material(0).setNormalTexture(new bg2e::base::Texture(assetPath, "rust_metal_normal.jpg"));
//        mainSphereDrawable->material(0).setMetalness(new bg2e::base::Texture(assetPath, "rust_metal_metallic.jpg"));
//        mainSphereDrawable->material(0).setRoughness(new bg2e::base::Texture(assetPath, "rust_metal_roughness.jpg"));
//        mainSphereDrawable->load(_engine);
//        
//        auto mainSphereNode = new bg2e::scene::Node("Main Sphere");
//        mainSphereNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(-5.0f, 0.0f, 4.0f));
//        mainSphereNode->transform()->scale(3.0f);
//        mainSphereNode->addComponent(new bg2e::scene::DrawableComponent(mainSphereDrawable));
//        sceneRoot->addChild(mainSphereNode);
//        
//        auto customSphereDrawable = std::make_shared<bg2e::scene::Drawable>();
//        customSphereDrawable->setMesh(sphereMesh);
//        customSphereDrawable->material(0).setMetalness(0.6f);
//        customSphereDrawable->material(0).setRoughness(0.2f);
//        customSphereDrawable->load(_engine);
//        _sphere = customSphereDrawable;
//        auto customSphereNode = new bg2e::scene::Node("Custom Sphere");
//        customSphereNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(5.0f, 0.0f, 4.0f));
//        customSphereNode->transform()->scale(3.0f);
//        customSphereNode->addComponent(new bg2e::scene::DrawableComponent(customSphereDrawable));
//        sceneRoot->addChild(customSphereNode);
//        
//        _engine->cleanupManager().push([&](VkDevice) {
//            _sphere.reset();
//        });
//       
//        return sceneRoot;
//    }
//    
//    std::shared_ptr<bg2e::scene::Node> scene2()
//    {
//        auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
//        sceneRoot->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "country_field_sun.jpg"));
//        
//        auto anotherNode = new bg2e::scene::Node("Transform Node");
//        anotherNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, -1.0f, 0.0f));
//        sceneRoot->addChild(anotherNode);
//        
//        auto drawable = std::shared_ptr<bg2e::scene::DrawableBase>(loadDrawable());
//        auto drawableComponent = std::make_shared<bg2e::scene::DrawableComponent>(drawable);
//        auto modelNode = std::make_shared<bg2e::scene::Node>("3D Model");
//        modelNode->addComponent(drawableComponent);
//        modelNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(2.0f, 0.0f, 0.0f));
//        anotherNode->addChild(modelNode);
//        
//        auto secondModel = new bg2e::scene::Node("Second 3D model");
//        auto anotherDrawable = new bg2e::scene::DrawableComponent(drawable);
//
//        secondModel->addComponent(anotherDrawable);
//        secondModel->addComponent(bg2e::scene::TransformComponent::makeTranslated(-2.0f, 0.0f, 0.0f ));
//        sceneRoot->addChild(secondModel);
//        
//        auto cameraNode = std::shared_ptr<bg2e::scene::Node>(new bg2e::scene::Node("Camera"));
//        cameraNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 2.5f, 10.0f ));
//        cameraNode->transform()->rotate(0.43, -1.0f, 0.0f, 0.0);
//        //cameraNode->addComponent(new bg2e::scene::TransformComponent());
//        //cameraNode->transform()->rotate(std::numbers::pi, 0.0f, 1.0f, 0.0f);
//        //cameraNode->transform()->rotate(0.13, 1.0f, 0.0f, 0.0f);
//        cameraNode->addComponent(new bg2e::scene::CameraComponent());
//        auto projection = new bg2e::math::OpticalProjection();
//        cameraNode->camera()->setProjection(projection);
//        
//        auto cameraRotation = new bg2e::scene::Node("Camera Rotation");
//        cameraRotation->addComponent(new bg2e::scene::TransformComponent());
//        //cameraRotation->transform()->rotate(std::numbers::pi, 0.0f, 1.0f, 0.0f);
//        cameraRotation->addComponent(new RotateCameraComponent(-0.002f));
//        cameraRotation->addChild(cameraNode);
//        sceneRoot->addChild(cameraRotation);
//        
//        // Lights: When you first create the scene in createScene(), you don't need to do anything with the lights, because the
//        // scene has not been initialised yet. Once the scene is initialised, if lights are added or removed during the rendering
//        // loop, _scene->updateLights() must be called to rebuild the array of lights in the scene that will be passed to the shader.
//        auto light1 = new bg2e::scene::Node("Light 1");
//        light1->addComponent(new bg2e::scene::LightComponent());
//        light1->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4 { 1.0f }, glm::vec3{ 4, 2, 4 } )));
//        light1->light()->light().setIntensity(3.7f);
//        sceneRoot->addChild(light1);
//        
//        auto light2 = new bg2e::scene::Node("Light 2");
//        light2->addComponent(new bg2e::scene::LightComponent());
//        light2->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4 { 1.0f }, glm::vec3{ 4,-2,-4} )));
//        light2->light()->light().setIntensity(0.3f);
//        sceneRoot->addChild(light2);
//        
//        auto light3 = new bg2e::scene::Node("Light 3");
//        light3->addComponent(new bg2e::scene::LightComponent());
//        light3->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4 { 1.0f }, glm::vec3{-4, 2, 4} )));
//        light3->light()->light().setIntensity(0.3f);
//        sceneRoot->addChild(light3);
//        
//        auto light4 = new bg2e::scene::Node("Light 4");
//        light4->addComponent(new bg2e::scene::LightComponent());
//        light4->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4 { 1.0f }, glm::vec3{-4,-2,-4} )));
//        light4->transform()->rotate(0.1, 0.0f, 1.0f, 0.0f);
//        light4->light()->light().setIntensity(0.3f);
//        sceneRoot->addChild(light4);
//        
//        
//        // Procedural room
//        auto roomSize = 10.0f;
//        auto planeMesh = std::shared_ptr<bg2e::geo::Mesh>(bg2e::geo::createPlane(roomSize, roomSize));
//        auto planeDrawable = std::make_shared<bg2e::scene::Drawable>();
//        planeDrawable->setMesh(planeMesh);
//        planeDrawable->material(0).setNormalTexture(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "normal_test.jpeg"));
//        planeDrawable->load(_engine);
//        
//        // Floor
//        auto floorNode = new bg2e::scene::Node("Floor");
//        floorNode->addComponent(new bg2e::scene::TransformComponent());
//        floorNode->transform()->translate(0.0f, roomSize * -0.5f, 0.0f);
//        floorNode->addComponent(new bg2e::scene::DrawableComponent(planeDrawable));
//        sceneRoot->addChild(floorNode);
//        
//        auto wall1Node = new bg2e::scene::Node("Wall1");
//        wall1Node->addComponent(new bg2e::scene::TransformComponent());
//        wall1Node->transform()->translate(0.0f, 0.0f, roomSize * -0.5f);
//        wall1Node->transform()->rotate(std::numbers::pi / 2.0f, 1.0f, 0.0f, 0.0f);
//        wall1Node->addComponent(new bg2e::scene::DrawableComponent(planeDrawable));
//        sceneRoot->addChild(wall1Node);
//        
//        auto wall2Node = new bg2e::scene::Node("Wall2");
//        wall2Node->addComponent(new bg2e::scene::TransformComponent());
//        wall2Node->transform()->translate(0.0f, 0.0f, roomSize * 0.5f);
//        wall2Node->transform()->rotate(std::numbers::pi / 2.0f, -1.0f, 0.0f, 0.0f);
//        wall2Node->addComponent(new bg2e::scene::DrawableComponent(planeDrawable));
//        sceneRoot->addChild(wall2Node);
//        
//        auto wall3Node = new bg2e::scene::Node("Wall3");
//        wall3Node->addComponent(new bg2e::scene::TransformComponent());
//        wall3Node->transform()->translate(roomSize * 0.5f, 0.0f, 0.0f);
//        wall3Node->transform()->rotate(std::numbers::pi / 2.0f, 0.0f, 1.0f, 0.0f);
//        wall3Node->transform()->rotate(std::numbers::pi / 2.0f, -1.0f, 0.0f, 0.0f);
//        wall3Node->addComponent(new bg2e::scene::DrawableComponent(planeDrawable));
//        sceneRoot->addChild(wall3Node);
//        
//        auto wall4Node = new bg2e::scene::Node("Wall4");
//        wall4Node->addComponent(new bg2e::scene::TransformComponent());
//        wall4Node->transform()->translate(roomSize * -0.5f, 0.0f, 0.0f);
//        wall4Node->transform()->rotate(std::numbers::pi / 2.0f, 0.0f, 1.0f, 0.0f);
//        wall4Node->transform()->rotate(std::numbers::pi / 2.0f, 1.0f, 0.0f, 0.0f);
//        wall4Node->addComponent(new bg2e::scene::DrawableComponent(planeDrawable));
//        sceneRoot->addChild(wall4Node);
//        
//        // Procedural geometry
//        auto cylinderMesh = bg2e::geo::createSphere(1.0f, 15, 15);
//        bg2e::geo::GenTangentsModifier<bg2e::geo::MeshPNUUT> genTangents(cylinderMesh);
//        genTangents.apply();
//        
//        auto cylinderDrawable = std::make_shared<bg2e::scene::Drawable>();
//        cylinderDrawable->setMesh(cylinderMesh);
//        cylinderDrawable->material(0).setNormalTexture(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "normal_test.jpeg"));
//        cylinderDrawable->material(0).setMetalness(1.0f);
//        cylinderDrawable->material(0).setRoughness(0.1f);
//        cylinderDrawable->load(_engine);
//        
//        auto cylinderNode = new bg2e::scene::Node("Cylinder");
//        cylinderNode->addComponent(new bg2e::scene::DrawableComponent(cylinderDrawable));
//        cylinderNode->addComponent(new bg2e::scene::TransformComponent());
//        cylinderNode->transform()->translate(0.0f, -2.0f, 2.0f);
//        sceneRoot->addChild(cylinderNode);
//        
//        auto cubeMesh = bg2e::geo::createCube(2.0f, 2.0f, 2.0f);
//        auto cubeDrawable = std::make_shared<bg2e::scene::Drawable>();
//        cubeDrawable->setMesh(cubeMesh);
//        
//        // Common material properties for all materials
//        cubeDrawable->iterateMaterials([](bg2e::base::MaterialAttributes & mat) {
//            mat.setMetalness(0.4f);
//            mat.setRoughness(0.1f);
//            mat.setAlbedo(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "logo_2a.png"));
//            mat.setNormalTexture(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "normal_test.jpeg"));
//        });
//        // Front face
//        cubeDrawable->material(0).setAlbedo(bg2e::base::Color::Red());
//        
//        // Back face
//        cubeDrawable->material(1).setAlbedo(bg2e::base::Color::Yellow());
//        
//        // Right face
//        cubeDrawable->material(2).setAlbedo(bg2e::base::Color::Green());
//        
//        // Left face
//        cubeDrawable->material(3).setAlbedo(bg2e::base::Color::Blue());
//        
//        // Top Face
//        cubeDrawable->material(3).setAlbedo(bg2e::base::Color::Bronze());
//
//        // Bottom face
//        cubeDrawable->material(5).setAlbedo(bg2e::base::Color::Coral());
//        cubeDrawable->load(_engine);
//        
//        auto cubeNode = new bg2e::scene::Node("Cube");
//        cubeNode->addComponent(new bg2e::scene::DrawableComponent(cubeDrawable));
//        cubeNode->addComponent(new bg2e::scene::TransformComponent());
//        cubeNode->transform()->translate(2.7f, -2.7f, 2.4f);
//        cubeNode->addComponent(new RotateCameraComponent(-0.02f));
//        sceneRoot->addChild(cubeNode);
//        
//        
//        return sceneRoot;
//    }
//    
//    std::shared_ptr<bg2e::scene::Node> createScene() override
//    {
//        return scene1();
//    }
//
//	bg2e::scene::DrawableBase * loadDrawable()
//	{  
//        std::filesystem::path modelPath = bg2e::base::PlatformTools::assetPath();
//        modelPath.append("two_submeshes.obj");
//        
//        auto innerAlbedoTexture = std::make_shared<bg2e::base::Texture>(
//            bg2e::base::PlatformTools::assetPath(),
//            "two_submeshes_inner_albedo.jpg"
//        );
//        auto innerNormalTexture = std::make_shared<bg2e::base::Texture>(
//            bg2e::base::PlatformTools::assetPath(),
//            "two_submeshes_inner_normal.jpg"
//        );
//        
//        auto outerAlbedoTexture = std::make_shared<bg2e::base::Texture>(
//            bg2e::base::PlatformTools::assetPath(),
//            "two_submeshes_outer_albedo.jpg"
//        );
//        auto outerNormalTexture = std::make_shared<bg2e::base::Texture>(
//            bg2e::base::PlatformTools::assetPath(),
//            "two_submeshes_outer_normal.jpg"
//        );
//        
//        auto drawable = new bg2e::scene::Drawable();
//        drawable->setMesh(bg2e::db::loadMeshObj<bg2e::geo::Mesh>(modelPath));
//        drawable->material(0).setAlbedo(outerAlbedoTexture);
//        drawable->material(0).setAlbedo(bg2e::base::Color::Red());
//        drawable->material(0).setNormalTexture(outerNormalTexture);
//        drawable->material(1).setAlbedo(innerAlbedoTexture);
//        drawable->material(1).setNormalTexture(innerNormalTexture);
//        drawable->load(_engine);
//        
//        return drawable;
//	}
//};
//
//class MyApplication : public bg2e::app::Application {
//public:
//	void init(int argc, char** argv) override
//	{
//		auto delegate = std::make_shared<BasicSceneDelegate>();
//		setRenderDelegate(delegate);
//		setInputDelegate(delegate);
//		setUiDelegate(delegate);
//	}
//};
//
//int main(int argc, char** argv) {
//	bg2e::app::MainLoop mainLoop;
//	MyApplication app;
//	app.init(argc, argv);
//	return mainLoop.run(&app);
//}

#include <bg2e.hpp>

#include <array>
#include <numbers>

class ClearScreenDelegate : public bg2e::render::RenderLoopDelegate,
    public bg2e::app::InputDelegate,
    public bg2e::ui::UserInterfaceDelegate
{
public:
    void init(bg2e::render::Engine * vulkan) override
    {
        using namespace bg2e::render::vulkan;
        RenderLoopDelegate::init(vulkan);
  
        // If you need to use the main descriptor set allocator, add all the required pool size ratios
        // in the init function.
        vulkan->descriptorSetAllocator().requirePoolSizeRatio(1, {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
        });
        
        _environment = std::unique_ptr<bg2e::render::EnvironmentResources>(
            new bg2e::render::EnvironmentResources(
                _engine,

                // Use this parameters to build the SkyboxRenderer in EnvironmentResources
                {
                    _engine->swapchain().imageFormat()
                },
                _engine->swapchain().depthImageFormat()
            )
        );
    }
 
    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator* frameAllocator) override
    {
        frameAllocator->requirePoolSizeRatio(1, {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 }
        });
        
        _environment->initFrameResources(frameAllocator);
        
        frameAllocator->initPool();
    }
 
    void initScene() override
    {
        // Use the initScene function to initialize and create scene resources, such as pipelines, 3D models
        // or textures
        
        auto assetsPath = bg2e::base::PlatformTools::assetPath();
        auto imagePath = assetsPath;
        imagePath.append("country_field_sun.jpg");
        
        _environment->build(
            imagePath,          // Path to equirectangular image
            { 2048, 2048 },     // Cube map size
            { 32, 32 },         // Irradiance map size
            { 1024, 1024 }      // Specular reflection map size
        );
    
        // You can use plain pointers in this case, because the base::Image and base::Texture objects will not
        // be used outside of this function. Internally, these objects will be stored in a shared_ptr and will be
        // managed by the render::Texture object.
        // But if you plan to use the objects more than once, you ALWAYS must to use a shared_ptr to share the pointer
        // between the Texture object and the rest of the application.
        auto cubeTexture = new bg2e::base::Texture(assetsPath, "logo_2a.png");
        cubeTexture->setMagFilter(bg2e::base::Texture::FilterLinear);
        cubeTexture->setMinFilter(bg2e::base::Texture::FilterLinear);
        cubeTexture->setUseMipmaps(true);
        
        // The render::Texture is used outside the function, for this reason we use managed pointers
        _cubeTexture = std::shared_ptr<bg2e::render::Texture>(new bg2e::render::Texture(
            _engine,
            cubeTexture
        ));
        
        _engine->cleanupManager().push([&](VkDevice) {
            _cubeTexture.reset();
        });
    
        createPipeline();

        _sceneData.viewMatrix = glm::lookAt(glm::vec3{ 0.0f, 0.0f, -5.0f}, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
        
        auto vpSize = _engine->swapchain().extent();
        _sceneData.projMatrix = glm::perspective(
            glm::radians(50.0f),
            float(vpSize.width) / float(vpSize.height),
            0.1f, 40.0f
        );
        _sceneData.projMatrix[1][1] *= -1.0f;

        _skyData.modelMatrix = glm::mat4{ 1.0f };
        _cubeData.modelMatrix = glm::mat4{ 1.0f };
        
        createVertexData();
    }

    void swapchainResized(VkExtent2D newExtent) override
    {
        _sceneData.projMatrix = glm::perspective(
            glm::radians(50.0f),
            float(newExtent.width) / float(newExtent.height),
            0.1f, 40.0f
        );
        _sceneData.projMatrix[1][1] *= -1.0f;
    }

    VkImageLayout render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        const bg2e::render::vulkan::Image* colorImage,
        const bg2e::render::vulkan::Image* depthImage,
        bg2e::render::vulkan::FrameResources& frameResources
    ) override {
        using namespace bg2e::render::vulkan;
  
        _environment->update(cmd, currentFrame, frameResources);
  
        // You can use this function when a descriptor set only contains one unique uniform buffer.
        // The uniformBufferDescriptorSet function automatically create the uniform buffer and descriptor set
        // inside the FrameResources object, upload the buffer data and update the descriptor set in one single
        // call. This function will also manage the allocated Vulkan memory and the heap memory. The descriptor
        // set object memory will be managed by the frameResources, so you don't need to delete it or manage
        // using a smart pointer. The object will be removed when the frame rendering is done.
        // You can save the pointer to the set descriptor and use it safely in other functions, as long as
        // they are used within the same frame.
        auto sceneDS = macros::uniformBufferDescriptorSet(
            _engine, frameResources,
            _sceneDSLayout, _sceneData, currentFrame
        );
        
        float flash = std::abs(std::sin(currentFrame / 120.0f));
        VkClearColorValue clearValue{ { 0.0f, 0.0f, flash, 1.0f } };
        macros::cmdClearImageAndBeginRendering(
            cmd,
            colorImage, clearValue, VK_IMAGE_LAYOUT_UNDEFINED,
            depthImage, 1.0f
        );
        
        macros::cmdSetDefaultViewportAndScissor(cmd, colorImage->extent2D());
  
        // Rotate the view along Y axis
        _sceneData.viewMatrix = glm::rotate(_sceneData.viewMatrix, 0.02f, glm::vec3(0.0f, 1.0f, 0.0f));

        // Draw skybox. This functions will only generate a skybox if the skybox renderer
        // is initialized in _environment. To do it, call de EnvironmentResources constructor
        // with the target image and depth format
        _environment->updateSkybox(_sceneData.viewMatrix, _sceneData.projMatrix);
        
        if (_drawSkybox)
        {
            _environment->drawSkybox(cmd, currentFrame, frameResources);
        }

  
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
  
        // Draw the cube
        _cubeData.modelMatrix = glm::rotate(_cubeData.modelMatrix, -0.025f, glm::vec3(1.0f, 1.0f, 0.0f));
        auto cubeDataBuffer = macros::createBuffer(_engine, frameResources, _cubeData);
        
        auto objectDS = frameResources.newDescriptorSet(_objectDSLayout);
        objectDS->beginUpdate();
            objectDS->addBuffer(
                0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                cubeDataBuffer, sizeof(ObjectData), 0
            );
            objectDS->addImage(
                1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                _cubeTexture->image()->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                _cubeTexture->sampler()
            );
            objectDS->addImage(
                2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                _environment->irradianceMapImage()->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                _environment->irradianceMapSampler()
            );
        objectDS->endUpdate();
        
        std::array<VkDescriptorSet, 2> sets = {
            sceneDS->descriptorSet(),
            objectDS->descriptorSet()
        };
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _layout, 0,
            uint32_t(sets.size()),
            sets.data(),
            0, nullptr
        );
        _cube->draw(cmd);
        
        _cylinderRotation += 0.015f;
        if (_cylinderRotation >= std::numbers::pi_v<float> * 2.0f)
        {
            _cylinderRotation = 0.0f;
        }
        _cylinderData.modelMatrix = glm::translate(glm::mat4{ 1.0f }, glm::vec3(2.0f, 1.5 * std::sin(0.01f * float(currentFrame)), 0.0f));
        _cylinderData.modelMatrix = glm::rotate(_cylinderData.modelMatrix, _cylinderRotation, glm::vec3(0.0f, 1.0f, 0.0f));
        auto planeDataBuffer = macros::createBuffer(_engine, frameResources, _cylinderData);
        
        auto cylinderDS = frameResources.newDescriptorSet(_objectDSLayout);
        cylinderDS->beginUpdate();
            cylinderDS->addBuffer(
                0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                planeDataBuffer, sizeof(ObjectData), 0
            );
            cylinderDS->addImage(
                1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
              _cubeTexture->image()->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              _cubeTexture->sampler()
            );
            cylinderDS->addImage(
                2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                _environment->irradianceMapImage()->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                _environment->irradianceMapSampler()
            );
        cylinderDS->endUpdate();
        
        std::array<VkDescriptorSet, 2> planeSets = {
            sceneDS->descriptorSet(),
            cylinderDS->descriptorSet()
        };
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _layout,
            0,
            uint32_t(planeSets.size()),
            planeSets.data(),
            0, nullptr
        );
        _cylinder->draw(cmd);
        
        _sphereData.modelMatrix = glm::translate(glm::mat4{1.0f}, glm::vec3(-2.0f, 1.5 * std::cos(0.01f * float(currentFrame)), 0.0f));
        auto sphereDataBuffer = macros::createBuffer(_engine, frameResources, _sphereData);
        
        auto sphereDS = frameResources.newDescriptorSet(_objectDSLayout);
        sphereDS->beginUpdate();
            sphereDS->addBuffer(
                0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                sphereDataBuffer, sizeof(ObjectData), 0
            );
            sphereDS->addImage(
                1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                _cubeTexture->image()->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                _cubeTexture->sampler()
            );
            sphereDS->addImage(
                2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                _environment->irradianceMapImage()->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                _environment->irradianceMapSampler()
            );
        sphereDS->endUpdate();
        
        std::array<VkDescriptorSet, 2> sphereSets = {
            sceneDS->descriptorSet(),
            sphereDS->descriptorSet()
        };
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _layout,
            0,
            uint32_t(sphereSets.size()),
            sphereSets.data(),
            0, nullptr
        );
        _sphere->draw(cmd);

        bg2e::render::vulkan::cmdEndRendering(cmd);

        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    // ============ User Interface Delegate Functions =========
    void init(bg2e::render::Engine *, bg2e::ui::UserInterface*) override {
        _window.setTitle("ImGui Wrapper Demo");
        _window.options.noClose = true;
        _window.options.minWidth = 100;
        _window.options.minHeight = 250;
        _window.setPosition(0, 0);
        _window.setSize(150, 270);
    }

    void drawUI() override
    {
        using namespace bg2e::ui;
        _window.draw([&]() {
            BasicWidgets::text("Hello, world!");
   
            BasicWidgets::checkBox("Draw Skybox", &_drawSkybox);
            
            if (BasicWidgets::button("Environment 1"))
            {
                loadEnvironment1();
            }
            
            if (BasicWidgets::button("Environment 2"))
            {
                loadEnvironment2();
            }
            
            if (BasicWidgets::button("Environment 3"))
            {
                loadEnvironment3();
            }
            
            if (BasicWidgets::button("Environment 4"))
            {
                loadEnvironment4();
            }
            
            if (BasicWidgets::button("Environment 5"))
            {
                loadEnvironment5();
            }
            
            if (BasicWidgets::button("Environment 6"))
            {
                loadEnvironment6();
            }
            
            if (BasicWidgets::button("Environment 7"))
            {
                loadEnvironment7();
            }
            
            if (BasicWidgets::button("Environment 8"))
            {
                loadEnvironment8();
            }
        });
    }
 
    void loadEnvironment1()
    {
        loadEnvironment("country_field_sun.jpg");
    }
    
    void loadEnvironment2()
    {
        loadEnvironment("equirectangular-env.jpg");
    }
    
    void loadEnvironment3()
    {
        loadEnvironment("equirectangular-env2.jpg");
    }
    
    void loadEnvironment4()
    {
        loadEnvironment("equirectangular-env3.jpg");
    }
    
    void loadEnvironment5()
    {
        loadEnvironment("equirectangular-env4.jpg");
    }
    
    void loadEnvironment6()
    {
        loadEnvironment("equirectangular-env5.jpg");
    }
    
    void loadEnvironment7()
    {
        loadEnvironment("equirectangular-env6.jpg");
    }
    
    void loadEnvironment8()
    {
        loadEnvironment("equirectangular-env7.jpg");
    }
    
    void loadEnvironment(const std::string& fileName)
    {
        auto assetsPath = bg2e::base::PlatformTools::assetPath();
        auto imagePath = assetsPath;
        imagePath.append(fileName);
        _environment->swapEnvironmentTexture(imagePath);
    }

protected:
    bg2e::ui::Window _window;

    VkPipelineLayout _layout;
    VkPipeline _pipeline;
 
    std::unique_ptr<bg2e::render::vulkan::geo::MeshPNU> _cube;
    std::unique_ptr<bg2e::render::vulkan::geo::MeshPNU> _cylinder;
    std::unique_ptr<bg2e::render::vulkan::geo::MeshPNU> _sphere;

    std::shared_ptr<bg2e::render::Texture> _cubeTexture;

    VkDescriptorSetLayout _sceneDSLayout;
    VkDescriptorSetLayout _objectDSLayout;

    // Load the environment skybox, generate the irradiance and specular reflection maps,
    // and manage the sky box renderer
    std::unique_ptr<bg2e::render::EnvironmentResources> _environment;
    
    bool _drawSkybox = true;
        
    struct SceneData
    {
        glm::mat4 viewMatrix;
        glm::mat4 projMatrix;
    };
    
    SceneData _sceneData;
    
    struct ObjectData
    {
        glm::mat4 modelMatrix;
    };
    ObjectData _skyData;
    ObjectData _cubeData;
    ObjectData _cylinderData;
    ObjectData _sphereData;
    float _cylinderRotation = 0.0f;

    void createPipeline()
    {
        bg2e::render::vulkan::factory::GraphicsPipeline plFactory(_engine);

        plFactory.addShader("test/environment_render_example.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        // plFactory.addShader("test/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        plFactory.addShader("test/environment_render_example.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        //plFactory.setInputBindingDescription(bg2e::render::vulkan::geo::MeshPNU::bindingDescription());
        //plFactory.setInputAttributeDescriptions(bg2e::render::vulkan::geo::MeshPNU::attributeDescriptions());
        plFactory.setInputState<bg2e::render::vulkan::geo::MeshPNU>();
  
        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;
        
        dsFactory.clear();
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        _sceneDSLayout = dsFactory.build(_engine->device().handle(), VK_SHADER_STAGE_VERTEX_BIT);
        
        dsFactory.clear();
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        dsFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        dsFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        _objectDSLayout = dsFactory.build(_engine->device().handle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        
        bg2e::render::vulkan::factory::PipelineLayout layoutFactory(_engine);
        layoutFactory.addDescriptorSetLayout(_sceneDSLayout);
        layoutFactory.addDescriptorSetLayout(_objectDSLayout);
        _layout = layoutFactory.build();
        
        plFactory.setDepthFormat(_engine->swapchain().depthImageFormat());
        plFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
        plFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        plFactory.setCullMode(true, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        plFactory.setColorAttachmentFormat(_engine->swapchain().imageFormat());
        _pipeline = plFactory.build(_layout);
  
        _engine->cleanupManager().push([&](VkDevice dev) {
            vkDestroyPipeline(dev, _pipeline, nullptr);
            vkDestroyPipelineLayout(dev, _layout, nullptr);
            vkDestroyDescriptorSetLayout(dev, _sceneDSLayout, nullptr);
            vkDestroyDescriptorSetLayout(dev, _objectDSLayout, nullptr);
        });
    }

    void createVertexData()
    {
        using namespace bg2e::render::vulkan;
  
        auto mesh = std::unique_ptr<bg2e::geo::MeshPNU>(
            bg2e::geo::createCubePNU(1.0f, 1.0f, 1.0f)
        );
        
        _cube = std::unique_ptr<bg2e::render::vulkan::geo::MeshPNU>(new bg2e::render::vulkan::geo::MeshPNU(_engine));
        _cube->setMeshData(mesh.get());
        _cube->build();
        
        mesh = std::unique_ptr<bg2e::geo::MeshPNU>(
            //bg2e::geo::createPlanePU(5.0f, 5.0f, false)
            bg2e::geo::createCylinderPNU(0.5f, 1.0f, 14, false)
        );
        
        _cylinder = std::unique_ptr<bg2e::render::vulkan::geo::MeshPNU>(new bg2e::render::vulkan::geo::MeshPNU(_engine));
        _cylinder->setMeshData(mesh.get());
        _cylinder->build();
        _cylinderData.modelMatrix = glm::translate(glm::mat4{ 1.0f }, glm::vec3(2.0f, 0.0f, 0.0f));
        
        mesh = std::unique_ptr<bg2e::geo::MeshPNU>(
            bg2e::geo::createSpherePNU(0.6f, 30, 30)
        );
        
        _sphere = std::unique_ptr<bg2e::render::vulkan::geo::MeshPNU>(new bg2e::render::vulkan::geo::MeshPNU(_engine));
        _sphere->setMeshData(mesh.get());
        _sphere->build();
        _sphereData.modelMatrix = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, -2.0f));
        

        _engine->cleanupManager().push([this](VkDevice dev) {
            _cube.reset();
            _cylinder.reset();
            _sphere.reset();
        });
    }
};

class MyApplication : public bg2e::app::Application {
public:
    void init(int argc, char** argv) override
    {
        auto delegate = std::shared_ptr<ClearScreenDelegate>(new ClearScreenDelegate());
        setRenderDelegate(delegate);
        setInputDelegate(delegate);
        setUiDelegate(delegate);
    }
};

int main(int argc, char** argv) {
    bg2e::app::MainLoop mainLoop;
    MyApplication app;
    app.init(argc, argv);
    return mainLoop.run(&app);
}
