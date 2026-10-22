/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <bg2e/db/scene.hpp>
#include <bg2e/db/image.hpp>
#include <bg2e/draw/MaterialBase.hpp>
#include <bg2e/geo/cube.hpp>
#include <bg2e/json/JsonParser.hpp>
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/scene/DrawableComponent.hpp>

#include <array>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace bg2e::draw::tests {
// Manual probe: call on a stopped, initialized draw Engine with no concurrent
// submissions, using an existing disposable directory. Nothing is auto-run.
inline void runCompleteResourceIoProbe(Engine& engine, const std::filesystem::path& directory)
{
    auto require = [](bool value)
    {
        if (!value)
        {
            throw std::runtime_error("Complete resource IO probe failed");
        }
    };
    const std::string name = "Model \"quoted\" \\ path\n";
    auto string = json::JSON(name);
    require(json::JsonParser(string->serialize()).parse()->stringValue() == name);
    require(json::JsonParser(string->toString()).parse()->stringValue() == name);

    auto drawable = std::make_shared<Drawable>();
    drawable->setName(name);
    drawable->setMesh(geo::createCube(1, 1, 1));
    drawable->load(&engine);
    auto node = std::make_shared<scene::Node>(name);
    node->addComponent(new scene::DrawableComponent(drawable)); // Deliberately no source path.
    gpu::ImageDescription description;
    description.size = {2, 2};
    description.format = gpu::PixelFormat::R32G32B32A32_SFLOAT;
    description.usage = gpu::ImageUsage::Sampled | gpu::ImageUsage::TransferSrc | gpu::ImageUsage::TransferDst;
    description.mipLevels = 2;
    auto image = engine.device()->createImage(description);
    const std::array<float, 16> mip0{4, 2, 1, 1, 8, 4, 2, 1, 16, 8, 4, 1, 32, 16, 8, 1};
    const std::array<float, 4> mip1{15, 7.5f, 3.75f, 1};
    image->upload({mip0.data(), sizeof(mip0), 0, 0, 0});
    image->upload({mip1.data(), sizeof(mip1), 0, 1, 0});
    auto settings = std::make_shared<base::Texture>();
    settings->setColorType(base::Color::TypeLinear);
    settings->setAddressMode(base::Texture::AddressModeClampToEdge);
    settings->setMinFilter(base::Texture::FilterNearest);
    auto texture = std::make_shared<Texture>(&engine);
    texture->load(settings, image);
    auto live = drawable->drawMaterial(0);
    live->setTexture(TextureSlot::AmbientOcclusion, texture);
    live->materialAttributes().setIndirectTextureMode(base::MaterialAttributes::IndirectTextureMode::BakedGlobalIllumination);
    live->materialAttributes().setAoUVSet(1);
    live->materialAttributes().setAoScale(glm::vec2(2, 3));
    const auto path = directory / "resource-round-trip.json";
    require(db::saveScene(node.get(), engine, path));
    require(node->getComponent<scene::DrawableComponent>()->resourcePath().empty());
    auto loaded = db::loadScene(path, engine);
    auto output = loaded->rootNode()->children().at(0)->getComponent<scene::DrawableComponent>()->drawable();
    const auto& material = output->material(0);
    require(output->name() == name && material.aoUVSet() == 1 && material.aoScale() == glm::vec2(2, 3));
    require(material.indirectTextureMode() == base::MaterialAttributes::IndirectTextureMode::BakedGlobalIllumination);
    require(material.aoTexture()->colorType() == base::Color::TypeLinear &&
        material.aoTexture()->addressModeU() == base::Texture::AddressModeClampToEdge);
    std::unique_ptr<base::Image> saved(db::loadImage(material.aoTexture()->imageFilePath()));
    require(saved->dataf() && saved->width() == 2 && saved->height() == 2);
    for (size_t i = 0; i < 4; ++i)
    {
        for (size_t c = 0; c < 3; ++c)
        {
            require(saved->dataf()[i * saved->channels() + c] == mip0[i * 4 + c]);
        }
    }
    require(!material.albedoTexture() && !material.normalTexture() && !material.roughnessTexture());
    size_t imageFiles = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.path().extension() == ".hdr" || entry.path().extension() == ".png")
        {
            ++imageFiles;
        }
    }
    require(imageFiles == 1); // Missing maps remain placeholders, not files.
    auto shared = output->drawMaterial(0);
    auto attributes = shared->materialAttributes();
    attributes.setLightEmissionTexture(attributes.aoTexture());
    shared->setMaterialAttributes(attributes);
    shared->updateTextures();
    require(db::saveScene(loaded->rootNode(), engine, directory / "shared.json"));
    size_t sharedImages = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.path().extension() == ".hdr")
        {
            ++sharedImages;
        }
    }
    require(sharedImages == 1); // Both slots reuse the existing source filename.

    // A failing custom serializer must leave the previous main and resources readable.
    class RejectExport final : public scene::Component {
      public:
        BG2E_COMPONENT_TYPE_NAME("RejectExport");

        std::shared_ptr<scene::Component> clone() const override
        {
            return std::make_shared<RejectExport>();
        }
    };
    const auto before = db::readSceneDocument(path)->serialize();
    node->addComponent(new RejectExport());
    require(!db::saveScene(node.get(), engine, path) && db::readSceneDocument(path)->serialize() == before &&
        node->getComponent<scene::DrawableComponent>()->resourcePath().empty());
    auto recovered = db::loadScene(path, engine);
    require(recovered->rootNode()->children().size() == 1);
    recovered->rootNode()->children().at(0)->getComponent<scene::DrawableComponent>()->drawable()->cleanup();
    engine.device()->waitIdle();
    output->cleanup();
    drawable->cleanup();
}

}
