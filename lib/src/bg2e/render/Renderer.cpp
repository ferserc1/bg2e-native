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

#include <bg2e/render/Renderer.hpp>
#include <bg2e/render/vulkan/rt/RayTracingScene.hpp>
#include <bg2e/base/Log.hpp>

namespace bg2e::render
{
void Renderer::updateScene(float delta, uint32_t maxLights)
{
    if (!_scene)
    {
        bg2e_log_warning << "Renderer::updateScene(): invalid scene" << bg2e_log_end;
        return;
    }

    _scene->willUpdate();

    _updateVisitor.update(_scene->rootNode(), delta);

    if (_scene->mainEnvironment() && _scene->mainEnvironment()->imgHash() != _skyImageHash)
    {
        _environment->swapEnvironmentTexture(_scene->mainEnvironment()->environmentImage());
        _skyImageHash = _scene->mainEnvironment()->imgHash();
    }

    // Update light resources
    auto lightComponents = _scene->lightComponents();
    auto lights = static_cast<uint32_t>(lightComponents.size() < maxLights
        ? lightComponents.size()
        : maxLights);
    _lightUniforms.lightCount = lights;
    for (uint32_t i = 0; i < lights; ++i)
    {
        auto comp = lightComponents[i];
        _lightUniforms.lights[i].type = comp->light().type();
        _lightUniforms.lights[i].color = comp->light().color();
        _lightUniforms.lights[i].intensity = comp->light().intensity();
        _lightUniforms.lights[i].position = comp->position();
        _lightUniforms.lights[i].direction = comp->direction();
        _lightUniforms.lights[i].spotAngle = comp->light().spotAngle();
        _lightUniforms.lights[i].spotCutoff = comp->light().spotCutoff();
    }

    _scene->didUpdate();
}

void Renderer::prepareSceneRender(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    render::vulkan::FrameResources& frameResources)
{
    using namespace bg2e::render::vulkan;
    _scene->willDraw();

    frameResources.rayTracingScene->update(cmd, _scene->rootNode());

    auto mainCamera = _scene->mainCamera();
    auto projMatrix = mainCamera->projectionMatrix();
    auto cameraWorldPos = mainCamera->ownerNode()->worldPosition();
    auto viewMatrix = mainCamera->ownerNode()->invertedWorldMatrix();

    _environment->update(cmd, currentFrame, frameResources);
    _environment->setSkyboxColorCorrection(_brightness, _contrast, _exposure);
    if (drawSkybox())
    {
        _environment->updateSkybox(viewMatrix, projMatrix);
    }
    // Create the render queue from the scene objeb8cts. This will initialize two queues
    // for opaque and transparent objects
    _renderQueueVisitor.enqueue(_scene->rootNode(), &_renderQueue);
}

void Renderer::endSceneRender()
{
    _scene->didDraw();
}

}

