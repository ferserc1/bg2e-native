
#include <bg2e.hpp>
#include <numbers>

class RotateCameraComponent : public bg2e::scene::Component {
public:
    RotateCameraComponent() :_r{0.001f} {}
    RotateCameraComponent(float r) :_r{r} {}
    
    void update(float delta) override
    {
        auto transform = ownerNode()->transform();
        
        if (transform)
        {
            transform->rotate(_r * delta / 10.0f, 0.0f, 1.0f, 0.0f);
        }
    }
    
protected:
    float _r;
};

class BasicSceneDelegate : public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererBasicForward>,
	public bg2e::app::InputDelegate,
	public bg2e::ui::UserInterfaceDelegate
{
public:
 
	// ============ User Interface Delegate Functions =========
	void init(bg2e::render::Engine*, bg2e::ui::UserInterface*) override {
		_window.setTitle("Basic forward renderer");
		_window.options.noClose = true;
		_window.options.minWidth = 190;
		_window.options.minHeight = 90;
		_window.setPosition(0, 0);
		_window.setSize(200, 100);
	}

	void drawUI() override
	{
        auto drawSkybox = renderer()->drawSkybox();
        _window.draw([&]() {
            bg2e::ui::BasicWidgets::checkBox("Draw Skybox", &drawSkybox);
        });
        renderer()->setDrawSkybox(drawSkybox);
	}

protected:
	bg2e::ui::Window _window;
    
    std::shared_ptr<bg2e::scene::Node> createScene() override
    {
        auto sceneRoot = std::make_shared<bg2e::scene::Node>("Scene Root");
        sceneRoot->addComponent(new bg2e::scene::EnvironmentComponent(bg2e::base::PlatformTools::assetPath(), "country_field_sun.jpg"));
        
        auto anotherNode = new bg2e::scene::Node("Transform Node");
        anotherNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, -1.0f, 0.0f));
        sceneRoot->addChild(anotherNode);
        
        auto drawable = std::shared_ptr<bg2e::scene::DrawableBase>(loadDrawable());
        auto drawableComponent = std::make_shared<bg2e::scene::DrawableComponent>(drawable);
        auto modelNode = std::make_shared<bg2e::scene::Node>("3D Model");
        modelNode->addComponent(drawableComponent);
        modelNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(2.0f, 0.0f, 0.0f));
        anotherNode->addChild(modelNode);
        
        auto secondModel = new bg2e::scene::Node("Second 3D model");
        auto anotherDrawable = new bg2e::scene::DrawableComponent(drawable);

        secondModel->addComponent(anotherDrawable);
        secondModel->addComponent(bg2e::scene::TransformComponent::makeTranslated(-2.0f, 0.0f, 0.0f ));
        sceneRoot->addChild(secondModel);
        
        auto cameraNode = std::shared_ptr<bg2e::scene::Node>(new bg2e::scene::Node("Camera"));
        cameraNode->addComponent(bg2e::scene::TransformComponent::makeTranslated(0.0f, 2.5f, 10.0f ));
        cameraNode->transform()->rotate(0.43, -1.0f, 0.0f, 0.0);
        //cameraNode->addComponent(new bg2e::scene::TransformComponent());
        //cameraNode->transform()->rotate(std::numbers::pi, 0.0f, 1.0f, 0.0f);
        //cameraNode->transform()->rotate(0.13, 1.0f, 0.0f, 0.0f);
        cameraNode->addComponent(new bg2e::scene::CameraComponent());
        auto projection = new bg2e::math::OpticalProjection();
        cameraNode->camera()->setProjection(projection);
        
        auto cameraRotation = new bg2e::scene::Node("Camera Rotation");
        cameraRotation->addComponent(new bg2e::scene::TransformComponent());
        //cameraRotation->transform()->rotate(std::numbers::pi, 0.0f, 1.0f, 0.0f);
        //cameraRotation->addComponent(new RotateCameraComponent());
        cameraRotation->addChild(cameraNode);
        sceneRoot->addChild(cameraRotation);
        
        // Lights: When you first create the scene in createScene(), you don't need to do anything with the lights, because the
        // scene has not been initialised yet. Once the scene is initialised, if lights are added or removed during the rendering
        // loop, _scene->updateLights() must be called to rebuild the array of lights in the scene that will be passed to the shader.
        auto light1 = new bg2e::scene::Node("Light 1");
        light1->addComponent(new bg2e::scene::LightComponent());
        light1->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4 { 1.0f }, glm::vec3{ 4, 2, 4 } )));
        light1->light()->light().setIntensity(5.0f);
        sceneRoot->addChild(light1);
        
        auto light2 = new bg2e::scene::Node("Light 2");
        light2->addComponent(new bg2e::scene::LightComponent());
        light2->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4 { 1.0f }, glm::vec3{ 2,-2, 4} )));
        sceneRoot->addChild(light2);
        
        auto light3 = new bg2e::scene::Node("Light 3");
        light3->addComponent(new bg2e::scene::LightComponent());
        light3->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4 { 1.0f }, glm::vec3{-2, 2, 4} )));
        sceneRoot->addChild(light3);
        
        auto light4 = new bg2e::scene::Node("Light 4");
        light4->addComponent(new bg2e::scene::LightComponent());
        light4->addComponent(new bg2e::scene::TransformComponent(glm::translate(glm::mat4 { 1.0f }, glm::vec3{-2,-2, 4} )));
        light4->transform()->rotate(0.1, 0.0f, 1.0f, 0.0f);
        sceneRoot->addChild(light4);
        
        
        // Procedural room
        auto roomSize = 10.0f;
        auto planeMesh = std::shared_ptr<bg2e::geo::Mesh>(bg2e::geo::createPlane(roomSize, roomSize));
        auto planeDrawable = std::make_shared<bg2e::scene::Drawable>();
        planeDrawable->setMesh(planeMesh);
        planeDrawable->material(0).setNormalTexture(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "normal_test.jpeg"));
        planeDrawable->load(_engine);
        
        // Floor
        auto floorNode = new bg2e::scene::Node("Floor");
        floorNode->addComponent(new bg2e::scene::TransformComponent());
        floorNode->transform()->translate(0.0f, roomSize * -0.5f, 0.0f);
        floorNode->addComponent(new bg2e::scene::DrawableComponent(planeDrawable));
        sceneRoot->addChild(floorNode);
        
        auto wall1Node = new bg2e::scene::Node("Wall1");
        wall1Node->addComponent(new bg2e::scene::TransformComponent());
        wall1Node->transform()->translate(0.0f, 0.0f, roomSize * -0.5f);
        wall1Node->transform()->rotate(std::numbers::pi / 2.0f, 1.0f, 0.0f, 0.0f);
        wall1Node->addComponent(new bg2e::scene::DrawableComponent(planeDrawable));
        sceneRoot->addChild(wall1Node);
        
        auto wall2Node = new bg2e::scene::Node("Wall2");
        wall2Node->addComponent(new bg2e::scene::TransformComponent());
        wall2Node->transform()->translate(0.0f, 0.0f, roomSize * 0.5f);
        wall2Node->transform()->rotate(std::numbers::pi / 2.0f, -1.0f, 0.0f, 0.0f);
        wall2Node->addComponent(new bg2e::scene::DrawableComponent(planeDrawable));
        sceneRoot->addChild(wall2Node);
        
        auto wall3Node = new bg2e::scene::Node("Wall3");
        wall3Node->addComponent(new bg2e::scene::TransformComponent());
        wall3Node->transform()->translate(roomSize * 0.5f, 0.0f, 0.0f);
        wall3Node->transform()->rotate(std::numbers::pi / 2.0f, 0.0f, 1.0f, 0.0f);
        wall3Node->transform()->rotate(std::numbers::pi / 2.0f, -1.0f, 0.0f, 0.0f);
        wall3Node->addComponent(new bg2e::scene::DrawableComponent(planeDrawable));
        sceneRoot->addChild(wall3Node);
        
        auto wall4Node = new bg2e::scene::Node("Wall4");
        wall4Node->addComponent(new bg2e::scene::TransformComponent());
        wall4Node->transform()->translate(roomSize * -0.5f, 0.0f, 0.0f);
        wall4Node->transform()->rotate(std::numbers::pi / 2.0f, 0.0f, 1.0f, 0.0f);
        wall4Node->transform()->rotate(std::numbers::pi / 2.0f, 1.0f, 0.0f, 0.0f);
        wall4Node->addComponent(new bg2e::scene::DrawableComponent(planeDrawable));
        sceneRoot->addChild(wall4Node);
        
        // Procedural geometry
        auto cylinderMesh = bg2e::geo::createSphere(1.0f, 15, 15);
        auto cylinderDrawable = std::make_shared<bg2e::scene::Drawable>();
        cylinderDrawable->setMesh(cylinderMesh);
        cylinderDrawable->material(0).setNormalTexture(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "normal_test.jpeg"));
        cylinderDrawable->load(_engine);
        
        auto cylinderNode = new bg2e::scene::Node("Cylinder");
        cylinderNode->addComponent(new bg2e::scene::DrawableComponent(cylinderDrawable));
        cylinderNode->addComponent(new bg2e::scene::TransformComponent());
        cylinderNode->transform()->translate(0.0f, -2.0f, 2.0f);
        sceneRoot->addChild(cylinderNode);
        
        auto cubeMesh = bg2e::geo::createCube(2.0f, 2.0f, 2.0f);
        auto cubeDrawable = std::make_shared<bg2e::scene::Drawable>();
        cubeDrawable->setMesh(cubeMesh);
        // Front face
        cubeDrawable->material(0).setAlbedo(bg2e::base::Color::Red());
        cubeDrawable->material(0).setAlbedo(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "logo_2a.png"));
        cubeDrawable->material(0).setNormalTexture(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "normal_test.jpeg"));
        
        // Back face
        cubeDrawable->material(1).setAlbedo(bg2e::base::Color::Yellow());
        cubeDrawable->material(1).setAlbedo(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "logo_2a.png"));
        cubeDrawable->material(1).setNormalTexture(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "normal_test.jpeg"));
        
        // Right face
        cubeDrawable->material(2).setAlbedo(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "logo_2a.png"));
        cubeDrawable->material(2).setAlbedo(bg2e::base::Color::Green());
        cubeDrawable->material(2).setNormalTexture(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "normal_test.jpeg"));
        
        // Left face
        cubeDrawable->material(3).setAlbedo(bg2e::base::Color::Blue());
        cubeDrawable->material(3).setAlbedo(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "logo_2a.png"));
        cubeDrawable->material(3).setNormalTexture(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "normal_test.jpeg"));
        
        // Top Face
        cubeDrawable->material(4).setAlbedo(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "logo_2a.png"));
        cubeDrawable->material(4).setNormalTexture(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "normal_test.jpeg"));
        
        // Bottom face
        cubeDrawable->material(5).setAlbedo(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "logo_2a.png"));
        cubeDrawable->material(5).setNormalTexture(new bg2e::base::Texture(bg2e::base::PlatformTools::assetPath(), "normal_test.jpeg"));
        cubeDrawable->load(_engine);
        
        auto cubeNode = new bg2e::scene::Node("Cube");
        cubeNode->addComponent(new bg2e::scene::DrawableComponent(cubeDrawable));
        cubeNode->addComponent(new bg2e::scene::TransformComponent());
        cubeNode->transform()->translate(2.7f, -2.7f, 2.4f);
        cubeNode->addComponent(new RotateCameraComponent(-0.02f));
        sceneRoot->addChild(cubeNode);
        
        
        return sceneRoot;
    }

	bg2e::scene::DrawableBase * loadDrawable()
	{  
        std::filesystem::path modelPath = bg2e::base::PlatformTools::assetPath();
        modelPath.append("two_submeshes.obj");
        
        auto innerAlbedoTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_inner_albedo.jpg"
        );
        auto innerNormalTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            //"two_submeshes_inner_normal.jpg"
            "normal_test.jpeg"
        );
        
        auto outerAlbedoTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            "two_submeshes_outer_albedo.jpg"
        );
        auto outerNormalTexture = std::make_shared<bg2e::base::Texture>(
            bg2e::base::PlatformTools::assetPath(),
            //"two_submeshes_outer_normal.jpg"
            "normal_test.jpeg"
        );
        
        auto drawable = new bg2e::scene::Drawable();
        drawable->setMesh(bg2e::db::loadMeshObj<bg2e::geo::Mesh>(modelPath));
        drawable->material(0).setAlbedo(outerAlbedoTexture);
        drawable->material(0).setAlbedo(bg2e::base::Color::Red());
        drawable->material(0).setNormalTexture(outerNormalTexture);
        drawable->material(1).setAlbedo(innerAlbedoTexture);
        drawable->material(1).setNormalTexture(innerNormalTexture);
        drawable->load(_engine);
        
        return drawable;
	}
};

class MyApplication : public bg2e::app::Application {
public:
	void init(int argc, char** argv) override
	{
		auto delegate = std::make_shared<BasicSceneDelegate>();
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
