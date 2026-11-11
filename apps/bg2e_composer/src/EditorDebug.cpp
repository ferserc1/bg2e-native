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

#include "EditorUi.hpp"
#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Group.hpp>
#include <bg2e/ui/Numeric.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace bg2e_composer {
void EditorUi::drawDebug(const std::shared_ptr<AppDelegate>& delegate)
{
    using namespace bg2e::draw;
    auto* renderer = dynamic_cast<RendererDeferred*>(delegate->renderer());
    if (!renderer)
    {
        bg2e::ui::Text::text("Buffer inspection requires the deferred renderer.");
        return;
    }
    if (_debugRendererGeneration != delegate->rendererGeneration())
    {
        _debugRendererGeneration = delegate->rendererGeneration();
        _debugOptions = renderer->debugOptions();
    }
    auto& options = _debugOptions;
    bool changed = bg2e::ui::Button::checkBox("Enabled",&options.enabled);
    bg2e::ui::Group::beginDisabled(!options.enabled);
    changed = bg2e::ui::Button::checkBox("Disable transparent layer",&options.disableTransparent) || changed;
    static const std::vector<std::string> views{
        "Final scene", "G-buffer: albedo", "G-buffer: normals", "G-buffer: roughness",
        "G-buffer: metallic", "Emission", "Depth (linear)", "Baked multiplier",
        "Direct lighting", "Indirect lighting", "Global illumination", "Ambient occlusion",
        "Reflections", "Reflection confidence", "Shadow visibility", "Environment irradiance",
        "Environment prefiltered specular", "Temporal acceptance / rejection", "Temporal motion",
        "Temporal current weight", "Temporal effective count", "Temporal reactivity", "Temporal variance",
        "Color before upscale", "Color after upscale", "G-buffer: material", "G-buffer: Fresnel / flags",
        "G-buffer: sheen / refraction"};
    uint32_t view = uint32_t(options.view);
    changed = bg2e::ui::Value::comboBox("Buffer",views,view) || changed;
    options.view = static_cast<RendererDebugView>(view);
    const bool effect = options.view == RendererDebugView::GlobalIllumination ||
        options.view == RendererDebugView::AmbientOcclusion || options.view == RendererDebugView::Reflections ||
        options.view == RendererDebugView::ReflectionConfidence;
    bg2e::ui::Group::beginDisabled(!effect);
    uint32_t stage = uint32_t(options.stage);
    changed = bg2e::ui::Value::comboBox("Stage",std::vector<std::string>{"Original (ray tracing)",
        "Accumulated (before denoise)","Filtered (denoise / spatial upsampling)"},stage) || changed;
    options.stage = static_cast<RendererDebugStage>(stage);
    bg2e::ui::Group::endDisabled();
    const bool composition = options.view == RendererDebugView::Final ||
        options.view == RendererDebugView::BeforeUpscale || options.view == RendererDebugView::AfterUpscale;
    bg2e::ui::Group::beginDisabled(composition);
    uint32_t layer = uint32_t(options.layer);
    changed = bg2e::ui::Value::comboBox("Layer",std::vector<std::string>{"Opaque","Transparent (last receiver)"},layer) || changed;
    options.layer = layer;
    bg2e::ui::Group::endDisabled();
    const bool temporal = options.view >= RendererDebugView::TemporalAcceptance && options.view <= RendererDebugView::TemporalVariance;
    bg2e::ui::Group::beginDisabled(!temporal);
    changed = bg2e::ui::Value::comboBox("Temporal signal",std::vector<std::string>{"Visibility (AO)",
        "Diffuse radiance (GI)","Reflected radiance"},options.temporalSignal) || changed;
    bg2e::ui::Group::endDisabled();
    if (options.view == RendererDebugView::ShadowVisibility)
    {
        int index = int(options.shadowIndex);
        changed = bg2e::ui::Numeric::sliderInt("Shadow index",&index,0,7) || changed;
        options.shadowIndex = uint32_t(index);
    }
    changed = bg2e::ui::Value::comboBox("Channels",std::vector<std::string>{"RGB","R","G","B","A"},options.channel) || changed;
    changed = bg2e::ui::Numeric::sliderFloat("Debug exposure (stops)",&options.exposure,-12,12) || changed;
    changed = bg2e::ui::Numeric::sliderFloat("Nearest zoom (center)",&options.zoom,1,16) || changed;
    if (options.view == RendererDebugView::Depth)
    {
        changed = bg2e::ui::Numeric::number("Depth minimum",&options.depthMinimum) || changed;
        changed = bg2e::ui::Numeric::number("Depth maximum",&options.depthMaximum) || changed;
        options.depthMinimum = std::isfinite(options.depthMinimum) ? std::clamp(options.depthMinimum,0.0f,1e30f) : 0.0f;
        const auto minimumMaximum = std::nextafter(options.depthMinimum,std::numeric_limits<float>::infinity());
        options.depthMaximum = std::isfinite(options.depthMaximum) ? std::max(minimumMaximum,options.depthMaximum) :
            std::max(minimumMaximum,10.0f);
    }
    if (bg2e::ui::Button::button("Final view"))
    {
        options.view = RendererDebugView::Final;
        changed = true;
    }
    bg2e::ui::Text::text(renderer->debugStatus());
    bg2e::ui::Text::text("Closing this window disables inspection. Settings are not saved.");
    bg2e::ui::Text::text("Transparent surfaces are omitted from rasterization and ray tracing when disabled.");
    if (temporal)
    {
        bg2e::ui::Text::text("Temporal views require the matching diagnostics / receiver metadata support.");
    }
    bg2e::ui::Group::endDisabled();
    if (changed)
    {
        const auto snapshot = options;
        delegate->enqueueRendererUpdate(_debugRendererGeneration,[snapshot](bg2e::draw::Renderer& target)
        {
            if (auto* deferred = dynamic_cast<RendererDeferred*>(&target))
            {
                deferred->setDebugOptions(snapshot);
            }
        });
    }
}
}
