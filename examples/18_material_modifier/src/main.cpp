#include <bg2e.hpp>
#include <bg2e/utils/MaterialModifier.hpp>
#include <bg2e/utils/MaterialSerializer.hpp>

#include <array>
#include <iostream>
#include <regex>

using bg2e::utils::MaterialModifier;

// The gallery is an interactive integration example, not a unit-test runner.
// Every sphere in the grid is a separate submesh of ONE Drawable, allowing
// the selection overloads to be inspected without scene-traversal machinery.
class MaterialModifierDelegate :
    public bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererDeferred>,
    public bg2e::app::InputDelegate,
    public bg2e::ui::UserInterfaceDelegate
{
    using Base = bg2e::render::DefaultRenderLoopDelegate<bg2e::render::RendererDeferred>;
    using Widgets = bg2e::ui::BasicWidgets;
    static constexpr uint32_t Side = 6;
    bg2e::ui::Window _window;
    bg2e::scene::InputVisitor _input;
    bg2e::scene::OrbitCameraComponent* _orbit = nullptr;
    std::shared_ptr<bg2e::scene::Drawable> _grid;
    std::array<std::shared_ptr<bg2e::scene::Drawable>, 4> _large;
    std::filesystem::path _assets;
    std::string _status = "Ready. Select a scenario below.";
    uint32_t _textureLoads = 0;

    void report(const std::string& text)
    {
        _status = text;
        std::cout << "[MaterialModifier] " << text << std::endl;
    }

    void resetGrid()
    {
        // Preserve the roughness/metalness gradient when recoloring at runtime.
        for (uint32_t i = 0; i < Side * Side; ++i)
        {
            bg2e::base::MaterialAttributes attributes;
            attributes.setAlbedo(bg2e::base::Color(0.8f, 0.18f, 0.08f, 1.0f));
            attributes.setMetalness(float(i / Side) / float(Side - 1));
            attributes.setRoughness(0.05f + 0.95f * float(i % Side) / float(Side - 1));
            _grid->setMaterial(attributes, i);
        }
        if (_grid->isLoaded()) _grid->updateMaterials();
    }

    MaterialModifier rust() const
    {
        // Exact native wire keys, including the historical AO spelling.
        return MaterialModifier(R"({
            "class":"PBRMaterial",
            "albedo":[1,1,1,1], "albedoTexture":"rust_metal_albedo.jpg",
            "albedoScale":[1,1], "albedoUV":0,
            "normalTexture":"rust_metal_normal.jpg", "normalScale":[1,1], "normalUV":0,
            "metalness":1, "metalnessTexture":"rust_metal_metallic.jpg",
            "metalnessScale":[1,1], "metalnessChannel":0, "metalnessInvert":false, "metalnessUV":0,
            "roughness":1, "roughnessTexture":"rust_metal_roughness.jpg",
            "roughnessScale":[1,1], "roughnessChannel":0, "roughnessInvert":false, "roughnessUV":0,
            "ambientOcclussion":"rust_metal_roughness.jpg",
            "ambientOcclussionScale":[1,1], "ambientOcclussionChannel":0, "ambientOcclussionUV":1,
            "fresnelTint":[1,0.9,0.8,1], "sheenIntensity":0.1, "sheenColor":[0.2,0.4,1,1],
            "lightEmission":0.05, "lightEmissionTexture":"rust_metal_albedo.jpg",
            "lightEmissionScale":[1,1], "lightEmissionChannel":1,
            "lightEmissionInvert":false, "lightEmissionUV":0,
            "isTransparent":false, "isSolid":true, "unlit":false, "refractionFactor":0.017
        })", _assets);
    }

    void invalidInputs()
    {
        const auto before = _grid->renderMaterial(0)->materialAttributes().roughness();
        const bool rejected =
            !MaterialModifier(std::string("{")).isValid() &&
            !MaterialModifier(std::string("[]")).isValid() &&
            !MaterialModifier(std::string("{} {}")).isValid() &&
            !MaterialModifier(std::string(R"({"roughness" 0.2})")).isValid() &&
            !MaterialModifier(std::string(R"({"roughness":0.2,})")).isValid() &&
            !MaterialModifier(std::string(R"({"albedo":[1,1,1,]})")).isValid() &&
            !MaterialModifier(std::string(R"({"class":"Other"})")).apply(*_grid, uint32_t{0}) &&
            !MaterialModifier(std::shared_ptr<bg2e::json::JsonNode>{}).isValid() &&
            !MaterialModifier(std::string("{}")).apply(*_grid, Side * Side) &&
            !MaterialModifier(std::string("{}")).apply(std::shared_ptr<bg2e::render::MaterialBase>{});
        MaterialModifier ignored(std::string(R"({
            "roughness":"wrong type", "albedo":[1,0], "albedoTexture":null,
            "metalnessChannel":-1, "normalUV":99, "unknownProperty":123,
            "name":"do not rename", "groupName":"do not regroup", "visible":false
        })"));
        ignored.apply(*_grid, uint32_t{0});
        const auto none = ignored.applyAll(*_grid, std::string("missing"));
        const auto noRegex = ignored.applyAll(*_grid, std::regex("^missing$"));
        report(rejected && none == 0 && noRegex == 0 &&
            _grid->renderMaterial(0)->materialAttributes().roughness() == before &&
            _grid->submeshGroupName(0) == "row-0" && _grid->submeshVisibility(0)
            ? "Invalid input ignored/rejected; grid unchanged."
            : "Unexpected result in invalid-input scenario.");
    }

public:
    void init(bg2e::render::Engine*, bg2e::ui::UserInterface*) override
    {
        _window.setTitle("18 - Material modifier");
        _window.options.noClose = true;
        _window.setPosition(12, 12);
        _window.setSize(440, 650);
    }

    void drawUI() override
    {
        _window.draw([&] {
            Widgets::text("Left drag: orbit | Right drag: pan | Wheel: zoom");
            Widgets::text("Grid: roughness left to right; metalness bottom to top.");
            Widgets::text("Large spheres, left to right:");
            Widgets::text("CPU rust | runtime rust | sheen / emission | glass");
            Widgets::text("Bottom two blue rows reuse one JSON-node snapshot.");
            if (Widgets::button("Reset camera")) _orbit->reset();
            if (Widgets::button("Reset grid")) { resetGrid(); report("Restored grid gradient."); }
            Widgets::separator("Submesh selection (color only)");
            if (Widgets::button("Single submesh: cyan"))
            {
                MaterialModifier(std::string(R"({"albedo":[0,0.8,1,1]})")).apply(*_grid, uint32_t{14});
                report("Only submesh 14 recolored; its gradient values remain.");
            }
            if (Widgets::button("Exact group row-2: green"))
            {
                auto n = MaterialModifier(std::string(R"({"albedo":[0.1,1,0.2,1]})"))
                    .applyAll(*_grid, std::string("row-2"));
                report("Exact group: " + std::to_string(n) + " submeshes (expected 6).");
            }
            if (Widgets::button("Regex row-[35]: gold"))
            {
                auto n = MaterialModifier(std::string(R"({"albedo":[1,0.65,0.08,1]})"))
                    .applyAll(*_grid, std::regex("row-[35]"));
                report("Regex: " + std::to_string(n) + " submeshes (expected 12).");
            }
            if (Widgets::button("All submeshes: blue"))
            {
                auto n = MaterialModifier(std::string(R"({"albedo":[0.08,0.3,1,1]})")).applyAll(*_grid);
                report("All: " + std::to_string(n) + " submeshes (expected 36).");
            }
            Widgets::separator("Runtime material / texture changes");
            if (Widgets::button("Apply rust to second large sphere"))
            {
                _textureLoads = 0;
                rust().apply(*_large[1], uint32_t{0}, [&] { ++_textureLoads; });
                report("Runtime rust: " + std::to_string(_textureLoads) + " texture callbacks (expected 6).");
            }
            if (Widgets::button("Scale / UV / channels only"))
            {
                MaterialModifier(std::string(R"({
                    "albedoScale":[3,2], "normalScale":[3,2],
                    "metalnessChannel":2, "roughnessInvert":true,
                    "albedoUV":1, "lightEmissionInvert":true
                })")).apply(*_large[1], uint32_t{0});
                report("Companion fields changed without supplying any texture.");
            }
            if (Widgets::button("Serializer JSON / absolute texture paths"))
            {
                bg2e::utils::MaterialSerializer serializer;
                std::vector<std::shared_ptr<bg2e::base::Texture>> textures;
                // Serialize the reference sphere. Absolute paths must not be
                // prefixed by the modifier's basePath.
                auto attributes = _large[0]->renderMaterial(0)->materialAttributes();
                const auto json = serializer.serializeMaterial(attributes, textures, false);
                MaterialModifier(json, _assets / "unused-base").applyAll(*_large[1]);
                report("Second sphere restored from native serializer JSON.");
            }
            if (Widgets::button("MaterialBase reference: sheen"))
            {
                MaterialModifier(std::string(R"({
                    "sheenIntensity":0.9, "sheenColor":[0.1,0.4,1,1],
                    "fresnelTint":[1,0.3,0.1,1], "lightEmission":0
                })")).apply(*_large[2]->renderMaterial(0));
                report("Third large sphere: direct MaterialBase reference.");
            }
            if (Widgets::button("Shared MaterialBase: unlit emission"))
            {
                MaterialModifier(std::string(R"({"unlit":true,"lightEmission":0.5})"))
                    .apply(_large[2]->renderMaterial(0));
                report("Third large sphere: shared MaterialBase, unlit + emission.");
            }
            if (Widgets::button("Explicit zero / false"))
            {
                MaterialModifier(std::string(R"({
                    "unlit":false,"lightEmission":0,"sheenIntensity":0,
                    "metalness":0,"roughness":0.2
                })")).apply(*_large[2], uint32_t{0});
                report("Zero and false are applied, not treated as missing.");
            }
            if (Widgets::button("Update materials + reload grid"))
            {
                _grid->updateMaterials();
                _grid->reload();
                report("Grid colors survive updateMaterials() and reload().");
            }
            if (Widgets::button("Preserve unrelated runtime edits"))
            {
                auto live = _grid->renderMaterial(0);
                live->materialAttributes().setRoughness(0.77f);
                MaterialModifier(std::string(R"({"albedo":[1,0,1,1]})")).apply(*_grid, uint32_t{0});
                report(live->materialAttributes().roughness() == 0.77f
                    ? "Runtime roughness 0.77 preserved; only albedo patched."
                    : "Unexpected change to runtime roughness.");
            }
            if (Widgets::button("Invalid input / unmatched groups")) invalidInputs();
            Widgets::separator("Last operation");
            Widgets::text(_status);
        });
    }

    void mouseMove(int x, int y) override { _input.mouseMove(renderer()->scene()->rootNode(), x, y); }
    void mouseButtonDown(int b, int x, int y) override { _input.mouseButtonDown(renderer()->scene()->rootNode(), b, x, y); }
    void mouseButtonUp(int b, int x, int y) override { _input.mouseButtonUp(renderer()->scene()->rootNode(), b, x, y); }
    void mouseWheel(int x, int y) override { _input.mouseWheel(renderer()->scene()->rootNode(), x, y); }

    void cleanup() override
    {
        _grid.reset();
        for (auto& drawable : _large) drawable.reset();
        Base::cleanup();
    }

protected:
    std::shared_ptr<bg2e::scene::Node> createScene() override
    {
        using namespace bg2e;
        _assets = base::PlatformTools::assetPath();
        auto root = std::make_shared<scene::Node>("Material Modifier Gallery");
        root->addComponent(new scene::EnvironmentComponent(_assets, "gothic_manor_01_4k.hdr"));

        auto sphere = std::shared_ptr<geo::Mesh>(geo::createSphere(0.8f, 35, 35));
        for (auto& vertex : sphere->vertices) vertex.texCoord1 = vertex.texCoord0 * 2.0f;
        geo::GenTangentsModifier<geo::Mesh> tangents(sphere.get());
        tangents.apply();
        auto gridMesh = std::make_shared<geo::Mesh>(*sphere);
        gridMesh->submeshes.assign(Side * Side, sphere->submeshes.front());
        _grid = std::make_shared<scene::Drawable>();
        _grid->setMesh(gridMesh);
        resetGrid();
        for (uint32_t i = 0; i < Side * Side; ++i)
        {
            _grid->setSubmeshGroupName("row-" + std::to_string(i / Side), i);
            _grid->setSubmeshName("Sphere-" + std::to_string(i), i);
            _grid->setSubmeshTransform(glm::translate(glm::mat4(1),
                glm::vec3((float(i % Side) - 2.5f) * 2.1f, float(i / Side) * 2.1f - 2.0f, 0)), i);
        }
        // A modifier built from a JSON node is a snapshot: mutating the input
        // afterwards must not change subsequent applications.
        auto nodeData = json::JSON(json::JsonObject{{"albedo", json::JSON(base::Color(0.2f, 0.6f, 0.9f, 1))}});
        MaterialModifier blue(nodeData);
        nodeData->objectValue()["albedo"] = json::JSON(base::Color::Red());
        blue.applyAll(*_grid, std::string("row-0")); // Before load.
        _grid->load(_engine);
        blue.applyAll(*_grid, std::string("row-1")); // Same modifier after load.
        auto gridNode = new scene::Node("Roughness and metalness matrix");
        gridNode->addComponent(new scene::DrawableComponent(_grid));
        root->addChild(gridNode);

        for (uint32_t i = 0; i < _large.size(); ++i)
        {
            auto drawable = std::make_shared<scene::Drawable>();
            drawable->setMesh(sphere);
            drawable->material().setRoughness(0.2f);
            drawable->material().setMetalness(0.6f);
            _large[i] = drawable;
            if (i == 0) rust().apply(drawable->material()); // CPU overload.
            drawable->load(_engine);
            auto node = new scene::Node("Large sphere " + std::to_string(i));
            node->addComponent(scene::TransformComponent::makeTranslated(-6.0f + float(i) * 4.0f, -5.0f, 2.0f));
            node->transform()->scale(2.0f);
            node->addComponent(new scene::DrawableComponent(drawable));
            root->addChild(node);
        }
        rust().applyAll(*_large[1]); // Runtime counterpart of the CPU sphere.
        MaterialModifier(std::string(R"({"albedo":[0.35,0.1,0.65,1],"sheenIntensity":0.7})"))
            .applyAll(*_large[2]);
        MaterialModifier(std::string(R"({
            "albedo":[0.4,0.8,1,0.35],"isTransparent":true,
            "isSolid":false,"metalness":0,"roughness":0.08,"refractionFactor":0.04
        })")).applyAll(*_large[3]);

        auto camera = new scene::Node("Camera");
        camera->addComponent(new scene::TransformComponent());
        camera->addComponent(new scene::CameraComponent());
        auto projection = new math::OpticalProjection();
        projection->setFar(500);
        projection->setFocalLength(35);
        camera->camera()->setProjection(projection);
        auto orbit = new scene::Node("Orbit");
        orbit->addComponent(new scene::TransformComponent());
        _orbit = new scene::OrbitCameraComponent();
        _orbit->setMaxDistance(100);
        _orbit->setMinDistance(3);
        _orbit->setInitialDistance(32);
        _orbit->setInitialCenter({0, 1, 0});
        _orbit->reset();
        orbit->addComponent(_orbit);
        orbit->addChild(camera);
        root->addChild(orbit);

        auto light = new scene::Node("Directional light");
        light->addComponent(new scene::TransformComponent());
        light->transform()->rotate(0.6f, 1, 0, 0);
        light->addComponent(new scene::LightComponent());
        light->light()->light().setType(base::Light::TypeDirectional);
        light->light()->light().setIntensity(3);
        root->addChild(light);
        return root;
    }
};

class MaterialModifierApplication : public bg2e::app::Application {
public:
    void init(int, char**) override
    {
        auto delegate = std::make_shared<MaterialModifierDelegate>();
        setRenderDelegate(delegate);
        setInputDelegate(delegate);
        setUiDelegate(delegate);
    }
};

int main(int argc, char** argv)
{
    bg2e::app::MainLoop loop("org.bg2engine.examples.material-modifier");
    MaterialModifierApplication app;
    app.init(argc, argv);
    return loop.run(&app);
}
