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

#include <bg2e/ui/Window.hpp>
#include <bg2e/draw/RenderSettingsPreferences.hpp>

namespace bg2e::ui {

// UI view over a shared live facade. Connect the facade to the delegate before
// its first renderer binding. No renderer pointers or configuration copies live
// here. UI calls/programmatic methods run on the render thread; edits are queued.
class BG2E_API GraphicsSettingsWindow : public Window {
  public:
    GraphicsSettingsWindow();
    ~GraphicsSettingsWindow() override;
    GraphicsSettingsWindow(const GraphicsSettingsWindow&) = delete;
    GraphicsSettingsWindow& operator=(const GraphicsSettingsWindow&) = delete;

    void bind(std::shared_ptr<draw::RenderSettingsPreferences> preferences);
    void clear();
    void draw() override;
    void queueEdit(draw::SettingHandle handle, std::any value);
    void importFile(const std::filesystem::path& path,
        draw::UnknownSettingPolicy policy = draw::UnknownSettingPolicy::Reject);
    void exportFile(const std::filesystem::path& path);
    void persist();
    void restore(draw::UnknownSettingPolicy policy = draw::UnknownSettingPolicy::Reject);
    bool pending() const;
    uint64_t catalogGeneration() const;
    const draw::SettingsResult& lastResult() const;

  protected:
    explicit GraphicsSettingsWindow(bool diagnostics);

  private:
    bool _diagnostics = false;
    struct State;
    std::shared_ptr<State> _state;
    std::shared_ptr<draw::RenderSettingsPreferences> _preferences;
    draw::RenderSettingsPreferences::Completion completion();
    void report(const std::string& code, const std::string& message);
    void drawContents();
};

// Session-only diagnostic controls over the same queued renderer settings.
class BG2E_API RendererDebugWidget : public GraphicsSettingsWindow {
  public:
    RendererDebugWidget();
};

}
