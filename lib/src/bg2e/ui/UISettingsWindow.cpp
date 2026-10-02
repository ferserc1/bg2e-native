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

#include <bg2e/ui/UISettingsWindow.hpp>
#include <bg2e/app/MainLoop.hpp>
#include <bg2e/app/PreferencesStore.hpp>
#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/manipulation/GizmoComponent.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Group.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Numeric.hpp>

#include <cmath>
#include <string>

namespace bg2e::ui {

namespace {
constexpr char preferencesContext[] = "app";
constexpr char backgroundLimitEnabledKey[] = "backgroundFrameRateLimitEnabled";
constexpr char backgroundMaxFrameRateKey[] = "backgroundMaxFrameRate";
}

void UISettingsWindow::init(bool showBackgroundFrameRateSettings)
{
    _showBackgroundFrameRateSettings = showBackgroundFrameRateSettings;
    setTitle("UI Settings");
    setSize(320, _showBackgroundFrameRateSettings ? 430 : 350);
    close();

    if (_showBackgroundFrameRateSettings)
    {
        if (auto* mainLoop = app::MainLoop::current())
        {
            auto& preferences = app::PreferencesStore::instance().preferences(preferencesContext);
            const auto storedFrameRate = preferences.get(
                backgroundMaxFrameRateKey,
                mainLoop->backgroundMaxFrameRate()
            );
            if (std::isfinite(storedFrameRate) && storedFrameRate > 0.0)
            {
                mainLoop->setBackgroundMaxFrameRate(storedFrameRate);
            }
            mainLoop->setBackgroundFrameRateLimitEnabled(preferences.get(
                backgroundLimitEnabledKey,
                mainLoop->backgroundFrameRateLimitEnabled()
            ));
        }
    }

    setDrawFunction([this]() {
        drawUI();
    });
}

void UISettingsWindow::drawUI()
{
    float scale = UserInterface::getScale();
    if (Numeric::sliderFloat("Interface Scale", &scale, 1.0f, 2.0f))
    {
        UserInterface::setScale(scale);
    }

    if (_showBackgroundFrameRateSettings)
    {
        drawBackgroundFrameRateSection();
    }

    Text::separator("Gizmos");

    using GizmoType = bg2e::manipulation::GizmoType;
    using GizmoComponent = bg2e::manipulation::GizmoComponent;

    struct GizmoTypeEntry {
        const char * label;
        GizmoType type;
    };
    static const GizmoTypeEntry types[] = {
        { "Camera",            GizmoType::Camera },
        { "Point Light",       GizmoType::PointLight },
        { "Spot Light",        GizmoType::SpotLight },
        { "Directional Light", GizmoType::DirectionalLight },
        { "Environment",       GizmoType::Environment },
    };

    for (auto& entry : types)
    {
        if (Group::collapsingHeader(entry.label))
        {
            bool visible = GizmoComponent::isGizmoVisible(entry.type);
            if (Button::checkBox(std::string("Visible##") + entry.label, &visible))
            {
                GizmoComponent::setGizmoVisible(entry.type, visible);
            }

            float opacity = GizmoComponent::gizmoOpacity(entry.type);
            if (Numeric::sliderFloat(std::string("Opacity##") + entry.label, &opacity, 0.0f, 1.0f))
            {
                GizmoComponent::setGizmoOpacity(entry.type, opacity);
            }

            float gizmoScale = GizmoComponent::gizmoScale(entry.type);
            if (Numeric::sliderFloat(std::string("Scale##") + entry.label, &gizmoScale, 0.01f, 0.5f))
            {
                GizmoComponent::setGizmoScale(entry.type, gizmoScale);
            }
        }
    }

    // Transform gizmo. Unlike the type gizmos above it is an independent facet,
    // with extra toggles to hide the scale handles (uniform / per-axis).
    if (Group::collapsingHeader("Transform"))
    {
        bool visible = GizmoComponent::isGizmoVisible(GizmoType::Transform);
        if (Button::checkBox("Visible##Transform", &visible))
        {
            GizmoComponent::setGizmoVisible(GizmoType::Transform, visible);
        }

        // No opacity control: the transform gizmo uses the opaque, depth-tested
        // pipeline (no blending), so opacity would have no visual effect.

        float gizmoScale = GizmoComponent::gizmoScale(GizmoType::Transform);
        if (Numeric::sliderFloat("Scale##Transform", &gizmoScale, 0.01f, 0.5f))
        {
            GizmoComponent::setGizmoScale(GizmoType::Transform, gizmoScale);
        }

        bool uniformScale = GizmoComponent::isScaleUniformVisible();
        if (Button::checkBox("Uniform scale control##Transform", &uniformScale))
        {
            GizmoComponent::setScaleUniformVisible(uniformScale);
        }

        bool axisScale = GizmoComponent::isScaleAxisVisible();
        if (Button::checkBox("Axis scale controls##Transform", &axisScale))
        {
            GizmoComponent::setScaleAxisVisible(axisScale);
        }
    }
}

void UISettingsWindow::drawBackgroundFrameRateSection()
{
    auto* mainLoop = app::MainLoop::current();
    if (!mainLoop) return;

    Text::separator("Background Performance");

    auto& preferences = app::PreferencesStore::instance().preferences(preferencesContext);

    bool enabled = mainLoop->backgroundFrameRateLimitEnabled();
    if (Button::checkBox("Limit frame rate when unfocused", &enabled))
    {
        mainLoop->setBackgroundFrameRateLimitEnabled(enabled);
        preferences.set(backgroundLimitEnabledKey, enabled);
    }

    float maxFrameRate = static_cast<float>(mainLoop->backgroundMaxFrameRate());
    if (Numeric::drag("Maximum background FPS", &maxFrameRate, 0.1f, 0.1f, 240.0f))
    {
        mainLoop->setBackgroundMaxFrameRate(static_cast<double>(maxFrameRate));
        preferences.set(backgroundMaxFrameRateKey, static_cast<double>(maxFrameRate));
    }
}

}
