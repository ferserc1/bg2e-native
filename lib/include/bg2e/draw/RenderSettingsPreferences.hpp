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

#include <bg2e/draw/SettingsCodec.hpp>
#include <bg2e/draw/DefaultRenderLoopDelegate.hpp>
#include <bg2e/app/Preferences.hpp>

namespace bg2e::draw {
// Stateless live facade. Direct bind/apply/restore require a stopped safe boundary.
// Persistence service is borrowed, optional, and must outlive this facade.
class BG2E_API RenderSettingsPreferences : public std::enable_shared_from_this<RenderSettingsPreferences> {
  public:
    using Completion = std::function<void(SettingsResult)>;
    using ExtendCatalog = std::function<void(Renderer&, SettingsCatalog&)>;
    explicit RenderSettingsPreferences(app::Preferences* persistence = nullptr,
        std::string key = "draw.settings.v1.hex");
    SettingsCatalog& catalog();
    const SettingsCatalog& catalog() const;
    void clear();
    void bind(Renderer& renderer, std::shared_ptr<scene::Node> root = {}, ExtendCatalog extensions = {});
    // Install before the delegate's first renderer binding. Weak callbacks never
    // keep the facade/delegate alive. Existing lifecycle callbacks must be composed
    // explicitly by callers who already own them; connect is an opt-in owner.
    void connect(const std::shared_ptr<DefaultRenderLoopDelegate>& delegate, ExtendCatalog extensions = {});
    void queueSet(SettingHandle handle, std::any value, Completion completion = {});
    void queueImport(const json::JsonNode& document, UnknownSettingPolicy policy = UnknownSettingPolicy::Reject,
        Completion completion = {});
    bool hasPersistence() const;
    void queueRestore(UnknownSettingPolicy policy = UnknownSettingPolicy::Reject, Completion completion = {});
    void persist() const;
    SettingsResult restore(const json::JsonNode* explicitDocument = nullptr,
        UnknownSettingPolicy policy = UnknownSettingPolicy::Reject);
    // Explicit document bypasses persisted defaults; otherwise persistence is used.
    // Missing properties retain their current live values. Nothing loads implicitly.
  private:
    void enqueue(SettingsImport plan, Completion completion);
    SettingsCatalog _catalog;
    app::Preferences* _persistence;
    std::string _key;
    std::weak_ptr<DefaultRenderLoopDelegate> _delegate;
    uint64_t _rendererGeneration = 0;
};
}
