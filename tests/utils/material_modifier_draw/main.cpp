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

#include <bg2e/utils/MaterialModifier.hpp>
#include <bg2e/geo/cube.hpp>
#include <stdexcept>
#include <utility>

namespace {
void require(bool value)
{
    if (!value)
    {
        throw std::runtime_error("Draw MaterialModifier contract check failed");
    }
}
}

int main()
{
    using namespace bg2e;
    draw::Drawable drawable;
    drawable.setMesh(geo::createCube(1.0f, 1.0f, 1.0f));
    require(drawable.submeshesCount() == 6);
    for (uint32_t i = 0; i < drawable.submeshesCount(); ++i)
    {
        drawable.setSubmeshName("face" + std::to_string(i), i);
        drawable.setSubmeshGroupName(i < 2 ? "body/front" : "body/back", i);
        drawable.materialDefinition(i).setMetalness(0.3f);
    }
    const utils::MaterialModifier patch(std::string{
        R"({"roughness":0,"alphaCutoff":0.2,"albedoUV":1,"roughnessScale":[2,3],"unlit":false})"});
    int callbacks = 0;
    require(patch.apply(drawable, "face0", [&callbacks]()
    {
        ++callbacks;
    }));
    const auto& definition = std::as_const(drawable).materialDefinition(0);
    require(&definition == &drawable.material(0)); // Only aliases before GPU realization.
    require(definition.roughness() == 0 && definition.alphaCutoff() == 0.2f);
    require(definition.metalness() == 0.3f && definition.albedoUVSet() == 1);
    require(definition.roughnessScale() == glm::vec2(2, 3));
    require(callbacks == 0);
    require(patch.applyAll(drawable, "body/front") == 2);
    require(patch.applyAll(drawable, std::regex("back")) == 4); // regex_search.
    require(patch.applyAll(drawable, "missing") == 0);
    require(!patch.apply(drawable, "missing"));
    require(!patch.apply(drawable, uint32_t(6)));
    require(!patch.apply(std::shared_ptr<draw::MaterialBase>{}));

    const utils::MaterialModifier noop(std::string{
        R"({"roughness":null,"alphaCutoff":"invalid","albedoUV":2,"metalnessChannel":4,"normalTexture":"","name":"ignored"})"});
    require(noop.isValid() && noop.applyAll(drawable) == 6);
    require(definition.roughness() == 0 && definition.alphaCutoff() == 0.2f);
    require(definition.albedoUVSet() == 1 && drawable.submeshName(0) == "face0");
    const utils::MaterialModifier invalid(std::string{R"({"class":"Other"})"});
    require(!invalid.isValid() && invalid.applyAll(drawable) == 0);
    const utils::MaterialModifier malformed(std::string{R"({"roughness":0,})"});
    require(!malformed.isValid());

    const utils::MaterialModifier texture(std::string{R"({"albedoTexture":"tex.png"})"}, "assets");
    require(texture.apply(drawable, uint32_t(0)));
    require(definition.albedoTexture()->imageFilePath() == "assets/tex.png");
    auto clone = drawable.clone();
    require(clone->materialDefinition(0).albedoTexture() != definition.albedoTexture());
    const utils::MaterialModifier clonePatch(std::string{R"({"roughness":0.8})"});
    require(clonePatch.apply(*clone, uint32_t(0)));
    require(definition.roughness() == 0 && clone->materialDefinition(0).roughness() == 0.8f);
    return 0;
}
