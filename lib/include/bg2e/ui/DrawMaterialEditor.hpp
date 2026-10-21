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

#include <bg2e/common.hpp>
#include <bg2e/draw/MaterialBase.hpp>
#include <bg2e/manipulation/SelectionManager.hpp>
#include <bg2e/utils/MaterialModifier.hpp>
#include <functional>
#include <memory>
#include <string>

namespace bg2e::draw {
class DefaultRenderLoopDelegate;
class Renderer;
}
namespace bg2e::ui {
class UserInterface;

// Queued material controls never mutate live
// targets during UI draw. Queued edits use weak targets and renderer generations.
class BG2E_API DrawMaterialEditor {
  public:
    DrawMaterialEditor();
    ~DrawMaterialEditor();
    DrawMaterialEditor(const DrawMaterialEditor&) = delete;
    DrawMaterialEditor& operator=(const DrawMaterialEditor&) = delete;

    void init(draw::Engine* engine, const std::shared_ptr<draw::DefaultRenderLoopDelegate>& delegate);
    void setSelectionManager(const std::shared_ptr<manipulation::SelectionManager>& selection);
    void setEditMaterial(const std::shared_ptr<draw::MaterialBase>& material);
    void cleanup();
    // Capture the owning document generation in this predicate. It runs before
    // queued work; rebinding the guard cancels earlier pending edits.
    void setEditGuard(std::function<bool()> guard);

    bool draw(UserInterface& ui);
    bool queuePatch(const utils::MaterialModifier& modifier, bool ordinaryAo = false);
    bool queueIndirectMode(base::MaterialAttributes::IndirectTextureMode mode);
    bool queueTextureSettings(draw::TextureSlot slot, std::function<void(base::Texture&)> edit);
    bool queueClearTexture(draw::TextureSlot slot);

    bool pending() const;
    const std::string& error() const;
    // Runs after edits at the safe boundary, including partial failure. The
    // default deferred upscaler reset is automatic; apps reset other histories here.
    void onChanged(std::function<void(draw::Renderer&)> callback);

  private:
    struct State;
    struct Impl;
    std::shared_ptr<State> _state;
    std::unique_ptr<Impl> _impl;

    bool queueEdit(std::function<void(draw::Drawable*, draw::MaterialBase*, uint32_t)> operation);
};

}
