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
#include "SceneLightmapWindow.hpp"

#include "AppDelegate.hpp"

#include <algorithm>

void SceneLightmapWindow::init(AppDelegate * delegate)
{
    _appDelegate = delegate;
    setTitle("Scene Lightmap Baker");
    _preview.init(delegate->engine());

    setDrawFunction([this]() {
        using namespace bg2e::ui;

        auto targets = collectTargets();
        if (targets.empty())
        {
            Text::text("The scene contains no Drawable nodes to bake.");
            return;
        }

        Text::separator("Bake Targets");
        for (const auto& node : targets)
        {
            bool selected = isSelected(node);
            const std::string label = node->name() + "##" + std::to_string(reinterpret_cast<uintptr_t>(node.get()));
            if (Button::checkBox(label, &selected))
            {
                setSelected(node, selected);
            }
        }

        Text::separator("Bake Settings");
        uint32_t mode = static_cast<uint32_t>(_mode);
        if (Value::comboBox("Mode", { "RTAO", "RTGI" }, mode))
        {
            _mode = static_cast<int>(mode);
        }
        Button::checkBox("RT Shadows", &_rtShadows);
        Numeric::sliderInt("Resolution", &_resolution, 64, 2048);
        Numeric::sliderInt("Accumulation Frames", &_frames, 1, 512);
        Numeric::sliderInt("Samples per Pixel", &_samples, 1, 256);
        Group::beginDisabled(_mode == 0);
        Numeric::sliderInt("GI Bounces", &_giBounces, 1, 8);
        Group::endDisabled();
        Numeric::drag("Max Ray Distance", &_maxDistance, 0.5f, 0.01f, 10000.0f);

        const bool baking = _appDelegate->lightmapBakeActive();
        if (Button::button("Generate Selected", false, baking))
        {
            std::vector<std::shared_ptr<bg2e::scene::Node>> nodes;
            for (const auto& weakNode : _selectedTargets)
            {
                if (auto node = weakNode.lock())
                {
                    nodes.push_back(node);
                }
            }
            if (nodes.empty())
            {
                _message = "Select at least one target.";
            }
            else
            {
                bg2e::render::LightmapSettings settings;
                settings.resolution = static_cast<uint32_t>(std::max(_resolution, 1));
                settings.accumulationFrames = static_cast<uint32_t>(std::max(_frames, 1));
                settings.samplesPerPixel = static_cast<uint32_t>(std::max(_samples, 1));
                settings.giBounces = static_cast<uint32_t>(std::max(_giBounces, 1));
                settings.maxRayDistance = std::max(_maxDistance, 0.01f);
                settings.mode = _mode == 1
                    ? bg2e::render::LightmapMode::RTGI
                    : bg2e::render::LightmapMode::RTAO;
                settings.rtShadows = _rtShadows;
                settings.cpuFormat = bg2e::render::LightmapPixelFormat::RGB8;
                try
                {
                    _message.clear();
                    _appDelegate->requestLightmapBake(nodes, settings);
                }
                catch (const std::exception& error)
                {
                    _message = error.what();
                    bg2e::app::MessageBox::showError("Scene Lightmap Baker", error.what());
                }
            }
        }

        if (baking)
        {
            const auto progress = _appDelegate->lightmapBakeProgress();
            Text::text(
                "Baking frame " + std::to_string(progress.first) +
                " / " + std::to_string(progress.second));
        }

        if (!_message.empty())
        {
            Text::text(_message);
        }

        const auto& resultPath = _appDelegate->lastLightmapPath();
        if (!resultPath.empty() && resultPath != _previewPath)
        {
            _previewPath = resultPath;
            _preview.setEditTexture(std::make_shared<bg2e::render::Texture>(
                _appDelegate->engine(),
                std::make_shared<bg2e::base::Texture>(_previewPath)));
        }
        if (!_previewPath.empty())
        {
            Text::separator("Last Result Preview");
            _preview.drawImage(192, 192);
        }
    });
}

void SceneLightmapWindow::cleanup()
{
    _preview.cleanup();
    _previewPath.clear();
    _selectedTargets.clear();
}

std::vector<std::shared_ptr<bg2e::scene::Node>> SceneLightmapWindow::collectTargets() const
{
    std::vector<std::shared_ptr<bg2e::scene::Node>> result;
    auto editableRoot = _appDelegate->stage()->editableRoot();
    if (!editableRoot)
    {
        return result;
    }

    std::vector<std::shared_ptr<bg2e::scene::Node>> stack { editableRoot };
    while (!stack.empty())
    {
        auto node = stack.back();
        stack.pop_back();
        auto* drawableComponent = node->drawable();
        if (drawableComponent &&
            std::dynamic_pointer_cast<bg2e::scene::Drawable>(drawableComponent->drawable()))
        {
            result.push_back(node);
        }
        for (const auto& child : node->children())
        {
            stack.push_back(child);
        }
    }
    return result;
}

bool SceneLightmapWindow::isSelected(const std::shared_ptr<bg2e::scene::Node>& node) const
{
    for (const auto& weakNode : _selectedTargets)
    {
        if (weakNode.lock() == node)
        {
            return true;
        }
    }
    return false;
}

void SceneLightmapWindow::setSelected(const std::shared_ptr<bg2e::scene::Node>& node, bool selected)
{
    if (selected)
    {
        if (!isSelected(node))
        {
            _selectedTargets.push_back(node);
        }
    }
    else
    {
        std::erase_if(_selectedTargets, [&](const std::weak_ptr<bg2e::scene::Node>& weakNode) {
            auto locked = weakNode.lock();
            return !locked || locked == node;
        });
    }
}
