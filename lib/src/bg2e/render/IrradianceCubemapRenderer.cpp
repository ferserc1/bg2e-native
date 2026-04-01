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

#include <bg2e/render/IrradianceCubemapRenderer.hpp>

namespace bg2e::render {

IrradianceCubemapRenderer::IrradianceCubemapRenderer(Engine * engine)
    :CubemapRenderer(engine)
{
}
    
void IrradianceCubemapRenderer::initFrameResources(vulkan::DescriptorSetAllocator* frameAllocator)
{
    CubemapRenderer::initFrameResources(frameAllocator);
}

void IrradianceCubemapRenderer::build(
    std::shared_ptr<vulkan::Image> inputCubemap,
    VkExtent2D /* cubeImageSize */
) {
    _viewTransform = glm::rotate(glm::mat4{ 1.0f }, glm::radians(180.0f), glm::vec3{ 0.0f, 1.0f, 0.0f });
    _viewTransform = glm::scale(_viewTransform, glm::vec3{ -1.0f, 1.0f, 1.0f });
    CubemapRenderer::build(
        inputCubemap,
        "cubemap_renderer.vert.spv",
        "irradiance_map_renderer.frag.spv",
        { 64, 64 },
        false,
        1,
        VK_FORMAT_R16G16B16A16_SFLOAT
    );
}

}
