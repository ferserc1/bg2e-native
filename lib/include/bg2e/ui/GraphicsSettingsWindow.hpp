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
  private:
    struct State;
    std::shared_ptr<State> _state;
    std::shared_ptr<draw::RenderSettingsPreferences> _preferences;
    draw::RenderSettingsPreferences::Completion completion();
    void report(const std::string& code, const std::string& message);
    void drawContents();
};
}
