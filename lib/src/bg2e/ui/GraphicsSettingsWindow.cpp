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

}

struct GraphicsSettingsWindow::State {
    draw::SettingsResult result;
    uint64_t generation = 0;
    uint32_t pending = 0;
    bool showUnavailable = true;
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
    Group::endDisabled();
    if (!_preferences->hasPersistence())
    {
        Text::text("Preference storage is not connected.");
    }
    Button::checkBox("Show unavailable settings", &_state->showUnavailable);
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
    auto descriptors = catalog.properties(_state->showUnavailable);
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

    for (const auto& component : components)
    {
        if (!Group::beginTree(componentLabel(component) + "##" + component))
        {
            continue;
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
                try
                {
                    auto value = catalog.read(descriptor.handle);
                    auto editor = descriptor.property;
                    if (editor.metadata.displayName.empty())
                    {
                        editor.metadata.displayName = readable(editor.name);
                    }
                    const bool sceneEdit = _sceneSession && (component.starts_with("environment/") || component.starts_with("light/"));
                    if (sceneEdit)
                    {
                        const auto key = component + "/" + descriptor.property.name;
                        auto change = std::any_cast<draw::SettingChange>(_sceneSession->value(key, draw::SettingChange{descriptor.handle, value}));
                        bool changed = false;
                        auto state = EditInteraction::draw([&]
                        {
                            changed = ReflectionWidget::editValue(change.value, editor, descriptor.available && !pending());
                        });
                        _sceneSession->submit(key, std::move(change), changed, state);
                    }
                    else if (ReflectionWidget::editValue(value, editor, descriptor.available && !pending()))
                    {
                        queueEdit(descriptor.handle, std::move(value));
                    }
                    if (!descriptor.available)
                    {
                        Text::text("Unavailable for the current configuration");
                    }
                }
                catch (const std::exception& error)
                {
                    Text::text(descriptor.property.name + ": " + error.what());
                }
            }
        }
        Group::endTree();
    }
    Group::popId();
}
}
