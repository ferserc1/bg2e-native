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


#include <bg2e/ui/GraphicsSettingsWindow.hpp>
#include <bg2e/ui/ReflectionWidget.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Group.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>
#include <imgui.h>
#include <charconv>
#include <limits>
#include <type_traits>
#include <bg2e/app/FileDialog.hpp>
#include <atomic>
#include <algorithm>
#include <cctype>
#include <exception>

namespace bg2e::ui {
namespace {

std::string readable(const std::string& name)
{
    std::string result;
    for (size_t i = 0; i < name.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(name[i]);
        if (c == '/' || c == '.' || c == '_' || c == '-')
        {
            result += ' ';
        }
        else
        {
            if (i && std::isupper(c) && std::islower(static_cast<unsigned char>(name[i - 1])))
            {
                result += ' ';
            }
            result += char(c);
        }
    }
    if (!result.empty())
    {
        result[0] = char(std::toupper(static_cast<unsigned char>(result[0])));
    }
    return result;
}

std::string componentLabel(const std::string& component)
{
    if (component.starts_with("upscaler/"))
    {
        return "Upscaling";
    }
    if (component.starts_with("environment/"))
    {
        return "Environment";
    }
    if (component.starts_with("light/"))
    {
        return "Light " + component.substr(6);
    }
    return readable(component);
}

// Window-specific scalar wrapper: precise bias formatting, integer distances,
// and clamped keyboard entry (Ctrl+click) without changing renderer metadata.
bool editGraphicsValue(std::any& value, const reflection::PropertyInfo& editor, bool enabled)
{
    const auto& name = editor.name;
    const bool integerDistance = name == "maxDistance";
    const bool scalarSlider = editor.editor == reflection::PropertyEditor::Slider &&
        (editor.type == reflection::PropertyType::UInt || editor.type == reflection::PropertyType::Float);
    if (name != "bias" && !integerDistance && name != "bloomLevels" && !scalarSlider)
    {
        return ReflectionWidget::editValue(value, editor, enabled);
    }
    Group::beginDisabled(!enabled);
    const auto label = editor.metadata.displayName + "##" + name;
    bool changed = false;
    if (name == "bias")
    {
        auto v = std::any_cast<float>(value);
        changed = ImGui::SliderFloat(label.c_str(), &v, 0.0002f, 0.02f,
            "%.4f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
        if (changed)
        {
            value = v;
        }
    }
    else if (integerDistance)
    {
        int v = int(std::clamp(std::any_cast<float>(value), 1.0f, 50000.0f));
        changed = ImGui::SliderInt(label.c_str(), &v, 1, 50000, "%d",
            ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
        if (changed)
        {
            value = float(v);
        }
    }
    else if (name == "bloomLevels")
    {
        const auto current = std::any_cast<uint32_t>(value);
        std::vector<std::string> labels{"4", "5", "6 (default)", "7", "8"};
        std::vector<uint32_t> levels{4, 5, 6, 7, 8};
        auto found = std::find(levels.begin(), levels.end(), current);
        uint32_t selected = uint32_t(found - levels.begin());
        if (found == levels.end())
        {
            labels.push_back(std::to_string(current) + " (current)");
            levels.push_back(current);
        }
        changed = Value::comboBox(label, labels, selected);
        if (changed)
        {
            value = levels[selected];
        }
    }
    else if (editor.type == reflection::PropertyType::UInt)
    {
        int v = int(std::any_cast<uint32_t>(value));
        changed = ImGui::SliderInt(label.c_str(), &v, int(*editor.metadata.min), int(*editor.metadata.max),
            "%d", ImGuiSliderFlags_AlwaysClamp);
        if (changed)
        {
            value = uint32_t(v);
        }
    }
    else
    {
        auto v = std::any_cast<float>(value);
        const auto flags = ImGuiSliderFlags_AlwaysClamp |
            (name == "directionalDistance" ? ImGuiSliderFlags_Logarithmic : ImGuiSliderFlags_None);
        changed = ImGui::SliderFloat(label.c_str(), &v, float(*editor.metadata.min), float(*editor.metadata.max),
            name == "directionalDistance" ? "%.0f" : "%.3f", flags);
        if (changed)
        {
            value = v;
        }
    }
    Text::tooltip(editor.metadata.tooltip);
    Group::endDisabled();
    return enabled && changed;
}

void configureEditor(reflection::PropertyInfo& editor, const std::string& component)
{
    auto range = [&](double min, double max)
    {
        editor.editor = reflection::PropertyEditor::Slider;
        editor.metadata.min = min;
        editor.metadata.max = max;
    };
    const auto& name = editor.name;
    if (editor.metadata.displayName.empty())
    {
        editor.metadata.displayName = readable(name);
    }
    if (name == "quality")
    {
        editor.metadata.displayName = "Quality (render resolution)";
        editor.metadata.tooltip = "Resolution of this effect; independent of the overall quality preset.";
    }
    if (name == "indirectLighting")
    {
        editor.metadata.enumOptions = {{"IBL", 0}, {"Ambient Occlusion", 1}, {"Global Illumination", 2}};
    }
    if (name == "bias")
    {
        range(0.0002, 0.02);
    }
    else if (name == "directionalDistance")
    {
        editor.metadata.displayName = "Max distance";
        range(100, 1000000);
        editor.metadata.tooltip = "Ctrl+click to enter a distance.";
    }
    else if (name == "maxSamples")
    {
        range(1, 32);
    }
    else if (name == "sampleCount")
    {
        range(component == "globalIllumination" ? 4 : component == "reflections" ? 2 : 1, 16);
    }
    else if (name == "shadowSamples" || name == "bounceCount")
    {
        range(1, 8);
    }
    else if (name == "maxDistance")
    {
        range(1, 50000);
    }
    else if (name == "radius")
    {
        range(0.05, 10);
    }
    else if (name == "falloff")
    {
        range(0.01, 5);
    }
    else if (name == "bounceAttenuation" || name == "maxRoughness")
    {
        range(0, 1);
    }
    else if (name == "roughnessSpread")
    {
        range(0, 4);
    }
    else if (name == "maxFrames")
    {
        range(10, 200);
    }
    else if (name == "kernelRadius")
    {
        range(1, 16);
    }
    else if (name == "absolutePositionThreshold" || name == "relativePositionThreshold" ||
        name == "minCurrentWeight" || name == "reactiveCurrentWeight")
    {
        range(0, 1);
    }
    else if (name == "normalThreshold")
    {
        range(-1, 1);
    }
    else if (name == "historyClipSigma")
    {
        range(0.01, 10);
    }
    else if (name == "depthThreshold" || name == "depthSigma" || name == "normalSigma")
    {
        range(0.001, 2);
    }
    else if (name == "spatialSigma")
    {
        range(0.1, 16);
    }
    else if (component == "final")
    {
        if (name == "exposure" || name == "brightness")
        {
            range(-1, 1);
        }
        else if (name == "contrast")
        {
            range(0, 2);
        }
        else if (name == "gamma")
        {
            range(0.1, 4);
        }
        else if (name == "bloomIntensity")
        {
            range(0, 2);
        }
        else if (name == "bloomThreshold")
        {
            range(0, 10);
        }
        else if (name == "bloomScatter")
        {
            range(0, 1);
        }
    }
}

}

struct GraphicsSettingsWindow::State {
    draw::SettingsResult result;
    uint64_t generation = 0;
    uint32_t pending = 0;
    bool temporalAdvanced = false;
    bool denoiseAdvanced = false;
};

GraphicsSettingsWindow::GraphicsSettingsWindow()
    : _state(std::make_shared<State>())
{
    static std::atomic<uint64_t> sequence{0};
    setTitle(std::string("Graphics settings##graphicsSettings") + std::to_string(++sequence));
    options.noMenu = true;
}

GraphicsSettingsWindow::~GraphicsSettingsWindow() = default;

void GraphicsSettingsWindow::bind(std::shared_ptr<draw::RenderSettingsPreferences> preferences)
{
    clear();
    _preferences = std::move(preferences);
}

void GraphicsSettingsWindow::clear()
{
    cancelInteraction();
    _state = std::make_shared<State>(); // Invalidates weak completion callbacks.
    _preferences.reset();
}

void GraphicsSettingsWindow::setSceneEditInteractionsEnabled(bool enabled)
{
    cancelInteraction();
    _sceneSession.reset();
    if (enabled)
    {
        _sceneSession = std::make_unique<EditSession>([this](EditPhase phase, uint64_t, const auto&, const std::any& payload)
        {
            if (phase == EditPhase::Cancel)
            {
                return true;
            }
            try
            {
                const auto& change = std::any_cast<const draw::SettingChange&>(payload);
                if (!_preferences || !_preferences->catalog().validate({change}).valid())
                {
                    return false;
                }
                if (phase == EditPhase::Commit)
                {
                    queueEdit(change.handle, change.value);
                }
                return true;
            }
            catch (...)
            {
                return false;
            }
        });
    }
}


bool GraphicsSettingsWindow::commitInteraction()
{
    return !_sceneSession || _sceneSession->commit();
}


void GraphicsSettingsWindow::cancelInteraction()
{
    if (_sceneSession)
    {
        _sceneSession->cancel();
    }
}


bool GraphicsSettingsWindow::pending() const
{
    return _state->pending != 0;
}

uint64_t GraphicsSettingsWindow::catalogGeneration() const
{
    return _preferences ? _preferences->catalog().generation() : 0;
}

const draw::SettingsResult& GraphicsSettingsWindow::lastResult() const
{
    return _state->result;
}

void GraphicsSettingsWindow::report(const std::string& code, const std::string& message)
{
    _state->result = {};
    _state->result.issues.push_back({code, "", message});
}

draw::RenderSettingsPreferences::Completion GraphicsSettingsWindow::completion()
{
    ++_state->pending;
    const std::weak_ptr<State> weak = _state;
    return [weak](draw::SettingsResult result)
    {
        if (auto state = weak.lock())
        {
            if (state->pending)
            {
                --state->pending;
            }
            state->result = std::move(result);
        }
    };
}

void GraphicsSettingsWindow::queueEdit(draw::SettingHandle handle, std::any value)
{
    if (!_preferences)
    {
        report("no_settings", "No graphics settings are connected");
        return;
    }
    _preferences->queueSet(std::move(handle), std::move(value), completion(),
        draw::SettingsUpdateSynchronization::ValueOnlyNotification);
}

void GraphicsSettingsWindow::queueChanges(std::vector<draw::SettingChange> changes)
{
    if (!_preferences || changes.empty())
    {
        return;
    }
    try
    {
        auto& catalog = _preferences->catalog();
        const auto checked = catalog.validate(changes);
        if (!checked.valid())
        {
            _state->result = checked;
            return;
        }
        // Use the existing atomic import queue, including its validation and
        // persistence path. Never call renderer setters while drawing the UI.
        json::JsonList components;
        std::map<std::string, json::JsonObject> properties;
        for (const auto& change : changes)
        {
            const auto descriptor = catalog.describe(change.handle);
            std::string type;
            std::shared_ptr<json::JsonNode> value;
            const auto number = [&]<typename T>(T v)
            {
                char buffer[128];
                std::to_chars_result result;
                if constexpr (std::is_floating_point_v<T>)
                {
                    result = std::to_chars(buffer, buffer + sizeof(buffer), v,
                        std::chars_format::general, std::numeric_limits<T>::max_digits10);
                }
                else
                {
                    result = std::to_chars(buffer, buffer + sizeof(buffer), v);
                }
                if (result.ec != std::errc{})
                {
                    throw std::runtime_error("Cannot encode graphics setting");
                }
                return json::JSON(std::string(buffer, result.ptr));
            };
            switch (descriptor.property.type)
            {
            case reflection::PropertyType::Bool:
                type = "bool";
                value = json::JSON(std::any_cast<bool>(change.value));
                break;
            case reflection::PropertyType::UInt:
                type = "uint32";
                value = number(std::any_cast<uint32_t>(change.value));
                break;
            case reflection::PropertyType::Float:
                type = "float";
                value = number(std::any_cast<float>(change.value));
                break;
            case reflection::PropertyType::Enum:
                type = "enum64";
                value = number(std::any_cast<int64_t>(change.value));
                break;
            default:
                throw std::runtime_error("Unsupported quality preset value type");
            }
            properties[change.handle.component][change.handle.property] = json::JSON(json::JsonObject{
                {"type", json::JSON(type)}, {"value", value}});
        }
        for (auto& [component, values] : properties)
        {
            const auto first = std::find_if(changes.begin(), changes.end(), [&](const auto& change)
            {
                return change.handle.component == component;
            });
            components.push_back(json::JSON(json::JsonObject{
                {"id", json::JSON(component)},
                {"type", json::JSON(catalog.describe(first->handle).componentType)},
                {"properties", json::JSON(std::move(values))}}));
        }
        const auto document = json::JSON(json::JsonObject{
            {"schema", json::JSON("bg2e.draw.settings")}, {"version", json::JSON("1")},
            {"renderer", json::JSON(catalog.rendererId())}, {"components", json::JSON(std::move(components))}});
        cancelInteraction();
        _preferences->queueImport(*document, draw::UnknownSettingPolicy::Reject, completion());
    }
    catch (const std::exception& error)
    {
        report("preset_failed", error.what());
    }
}

void GraphicsSettingsWindow::applyPreset(size_t index)
{
    if (!_preferences || pending() || index >= graphicsQualityPresets.size())
    {
        return;
    }
    const auto& preset = graphicsQualityPresets[index];
    const auto descriptors = _preferences->catalog().properties(true);
    std::vector<draw::SettingChange> changes;
    for (const auto& setting : preset.settings)
    {
        const auto found = std::find_if(descriptors.begin(), descriptors.end(), [&](const auto& descriptor)
        {
            return descriptor.handle.component == setting.component && descriptor.property.name == setting.property;
        });
        if (found != descriptors.end() && found->available && !found->property.isReadOnly())
        {
            changes.push_back({found->handle, std::visit([](auto value) -> std::any
            {
                return value;
            }, setting.value)});
        }
    }
    const auto method = std::find_if(descriptors.begin(), descriptors.end(), [](const auto& descriptor)
    {
        return descriptor.handle.component == "upscaling" && descriptor.property.name == "method";
    });
    if (method != descriptors.end())
    {
        const auto& options = method->property.metadata.enumOptions;
        const auto supported = std::find_if(preset.upscaleMethods.begin(), preset.upscaleMethods.end(), [&](auto candidate)
        {
            return std::any_of(options.begin(), options.end(), [&](const auto& option)
            {
                return option.second == candidate;
            });
        });
        if (supported == preset.upscaleMethods.end())
        {
            report("upscaling_unavailable", "This device supports neither MetalFX nor FSR; the preset was not applied.");
            return;
        }
        changes.push_back({method->handle, *supported});
    }
    queueChanges(std::move(changes));
}


void GraphicsSettingsWindow::importFile(const std::filesystem::path& path, draw::UnknownSettingPolicy policy)
{
    if (!_preferences)
    {
        report("no_settings", "No graphics settings are connected");
        return;
    }
    try
    {
        const auto document = draw::SettingsCodec::readFile(path);
        _preferences->queueImport(*document, policy, completion());
    }
    catch (const std::exception& error)
    {
        report("import_failed", error.what());
    }
}

void GraphicsSettingsWindow::exportFile(const std::filesystem::path& path)
{
    if (!_preferences || pending())
    {
        report("settings_busy", "Connect settings and apply pending changes before exporting");
        return;
    }
    try
    {
        draw::SettingsCodec::exportFile(_preferences->catalog(), path, !_preferences->hasSceneEditOwner());
        _state->result = {};
    }
    catch (const std::exception& error)
    {
        report("export_failed", error.what());
    }
}

void GraphicsSettingsWindow::persist()
{
    if (!_preferences || pending())
    {
        report("settings_busy", "Connect settings and apply pending changes before saving preferences");
        return;
    }
    try
    {
        _preferences->persist();
        _state->result = {};
    }
    catch (const std::exception& error)
    {
        report("persistence_failed", error.what());
    }
}

void GraphicsSettingsWindow::restore(draw::UnknownSettingPolicy policy)
{
    if (!_preferences)
    {
        report("no_settings", "No graphics settings are connected");
        return;
    }
    _preferences->queueRestore(policy, completion());
}

void GraphicsSettingsWindow::draw()
{
    if (_sceneSession)
    {
        _sceneSession->beginFrame();
    }
    Window::draw([this]
    {
        drawContents();
    });
    if (_sceneSession)
    {
        _sceneSession->endFrame();
    }
}

void GraphicsSettingsWindow::drawContents()
{
    if (!_preferences || _preferences->catalog().rendererId().empty())
    {
        Text::text("Graphics settings are waiting for an active renderer.");
        return;
    }
    auto& catalog = _preferences->catalog();
    if (_state->generation != catalog.generation())
    {
        // No old descriptors/values survive a rebind. A fresh generation also
        // changes the UI ID scope, discarding in-progress native editor state.
        cancelInteraction();
        _state->generation = catalog.generation();
    }
    Group::pushId(int(_state->generation & 0x7fffffff));
    Group::beginDisabled(pending());
    if (Button::button("Restore preferences from file"))
    {
        app::FileDialog picker;
        picker.setFilters({{"Graphics settings", "json"}});
        const auto path = picker.openFile();
        if (!path.empty())
        {
            importFile(path);
        }
    }
    if (Button::button("Save preferences to file", true))
    {
        app::FileDialog picker;
        picker.setFilters({{"Graphics settings", "json"}});
        const auto path = picker.saveFile();
        if (!path.empty())
        {
            exportFile(path);
        }
    }
    std::vector<std::string> presetNames{"Select a preset..."};
    for (const auto& preset : graphicsQualityPresets)
    {
        presetNames.emplace_back(preset.name);
    }
    // An action selector allows reapplying the same preset after external edits.
    uint32_t selectedPreset = 0;
    if (Value::comboBox("Quality preset", presetNames, selectedPreset) && selectedPreset > 0)
    {
        applyPreset(selectedPreset - 1);
    }
    Group::endDisabled();
    if (!_preferences->hasPersistence())
    {
        Text::text("Preference storage is not connected.");
    }
    if (pending())
    {
        Text::text("Applying settings before the next scene frame...");
    }
    for (const auto& issue : _state->result.issues)
    {
        Text::text(std::string(issue.error ? "Error: " : "Notice: ") + issue.path + " " + issue.message);
    }

    // Availability can change without a generation change (e.g. light type), so
    // enumerate every frame. Only handles/metadata and transient values are used.
    auto descriptors = catalog.properties(false);
    std::erase_if(descriptors, [](const auto& descriptor)
    {
        return !draw::SettingsCatalog::isGraphicsSetting(descriptor);
    });
    std::vector<std::string> components;
    for (const auto& descriptor : descriptors)
    {
        if (std::find(components.begin(), components.end(), descriptor.handle.component) == components.end())
        {
            components.push_back(descriptor.handle.component);
        }
    }

    std::stable_partition(components.begin(), components.end(), [](const auto& component)
    {
        return component != "final";
    });
    int64_t indirectLighting = 0;
    bool freezeAtLimit = false;
    for (const auto& descriptor : descriptors)
    {
        if (descriptor.handle.component == "renderer" && descriptor.property.name == "indirectLighting")
        {
            indirectLighting = std::any_cast<int64_t>(catalog.read(descriptor.handle));
        }
        if (descriptor.handle.component == "temporal" && descriptor.property.name == "freezeAtLimit")
        {
            freezeAtLimit = std::any_cast<bool>(catalog.read(descriptor.handle));
        }
    }
    for (const auto& component : components)
    {
        if ((component == "ambientOcclusion" && indirectLighting != 1) ||
            (component == "globalIllumination" && indirectLighting != 2))
        {
            continue;
        }
        const auto label = component == "final" ? "Appearance: Bloom and color correction" : componentLabel(component);
        if (!Group::beginTree(label + "##" + component))
        {
            continue;
        }
        Group::pushId(component);
        if (component == "temporal")
        {
            Button::checkBox("Show advanced", &_state->temporalAdvanced);
        }
        if (component == "denoise")
        {
            Button::checkBox("Show advanced", &_state->denoiseAdvanced);
        }
        std::vector<std::string> categories;
        for (const auto& descriptor : descriptors)
        {
            const auto& category = descriptor.property.metadata.category;
            if (descriptor.handle.component == component && std::find(categories.begin(), categories.end(), category) == categories.end())
            {
                categories.push_back(category);
            }
        }
        if (component == "final")
        {
            std::stable_partition(categories.begin(), categories.end(), [](const auto& category)
            {
                return category != "Color";
            });
        }
        for (const auto& category : categories)
        {
            if (!category.empty())
            {
                Text::separator(category);
            }
            for (const auto& descriptor : descriptors)
            {
                if (descriptor.handle.component != component || descriptor.property.metadata.category != category)
                {
                    continue;
                }
                const auto& name = descriptor.property.name;
                if (((component == "ambientOcclusion" || component == "globalIllumination") && name == "enabled") ||
                    name == "useBlueNoise" ||
                    (component == "temporal" && !_state->temporalAdvanced && name != "enabled" &&
                        name != "freezeAtLimit" && name != "maxFrames" && name != "adaptiveEnabled") ||
                    (component == "denoise" && !_state->denoiseAdvanced && name != "enabled" && name != "kernelRadius"))
                {
                    continue;
                }
                try
                {
                    auto value = catalog.read(descriptor.handle);
                    auto editor = descriptor.property;
                    configureEditor(editor, component);
                    const bool editable = descriptor.available && !pending() &&
                        !(component == "temporal" && name == "maxFrames" && !freezeAtLimit);
                    const bool sceneEdit = _sceneSession && (component.starts_with("environment/") || component.starts_with("light/"));
                    if (sceneEdit)
                    {
                        const auto key = component + "/" + descriptor.property.name;
                        auto change = std::any_cast<draw::SettingChange>(_sceneSession->value(key, draw::SettingChange{descriptor.handle, value}));
                        bool changed = false;
                        auto state = EditInteraction::draw([&]
                        {
                            changed = editGraphicsValue(change.value, editor, editable);
                        });
                        _sceneSession->submit(key, std::move(change), changed, state);
                    }
                    else if (editGraphicsValue(value, editor, editable))
                    {
                        if (component == "renderer" && name == "indirectLighting")
                        {
                            std::vector<draw::SettingChange> changes{{descriptor.handle, value}};
                            const auto selected = std::any_cast<int64_t>(value);
                            for (const auto& effect : {"ambientOcclusion", "globalIllumination"})
                            {
                                if ((selected == 1 && std::string(effect) == "ambientOcclusion") ||
                                    (selected == 2 && std::string(effect) == "globalIllumination"))
                                {
                                    for (const auto& candidate : descriptors)
                                    {
                                        if (candidate.handle.component == effect && candidate.property.name == "enabled")
                                        {
                                            changes.push_back({candidate.handle, true});
                                        }
                                    }
                                }
                            }
                            queueChanges(std::move(changes));
                        }
                        else
                        {
                            queueEdit(descriptor.handle, std::move(value));
                        }
                    }
                }
                catch (const std::exception& error)
                {
                    Text::text(descriptor.property.name + ": " + error.what());
                }
            }
        }
        Group::popId();
        Group::endTree();
    }
    Group::popId();
}
}
