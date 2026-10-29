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
#include <bg2e/ui/EditInteraction.hpp>
#include <optional>
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

// A typed partial sampler edit. Unspecified fields preserve each target's
// own settings; it never carries an opaque operation on a live resource.
struct BG2E_API TextureSamplingEdit {
    std::optional<base::Color::Type> colorType;
    std::optional<base::Texture::Filter> magFilter;
    std::optional<base::Texture::Filter> minFilter;
    std::optional<base::Texture::AddressMode> addressU;
    std::optional<base::Texture::AddressMode> addressV;
    std::optional<bool> mipmaps;
    std::optional<float> lodBias;

    void apply(base::Texture& texture) const;
};

struct MaterialEditIntent {
    std::optional<utils::MaterialModifier> patch;
    bool ordinaryAo = false;
    bool cpuValuesOnly = false;
    int linearSlot = -1;
    int clearSlot = -1;
    std::optional<base::MaterialAttributes::IndirectTextureMode> indirectMode;
    std::optional<TextureSamplingEdit> sampling;
    draw::TextureSlot samplingSlot = draw::TextureSlot::Albedo;
};

struct MaterialEditTargets {
    std::vector<manipulation::SelectionItem> selection;
    std::weak_ptr<draw::MaterialBase> direct;
    uint64_t binding = 0;
    uint64_t rendererGeneration = 0;
};

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

    // Opt in only when onChanged obeys the CPU value update contract. Resource
    // work in notifications, such as bake cancellation, must synchronize itself.
    // Texture edits require the separate retained-resource opt-in below.
    void setCpuValueEditsEnabled(bool enabled);

    // Opt in only when onChanged is also safe without a global drain. Replaced
    // texture bindings are retired against their outstanding submissions.
    void setRetainedResourceEditsEnabled(bool enabled);

    // Optional exclusive mutation owner. Receives immutable values before any
    // live edit, with targets frozen on Begin. No internal edit queue or changed
    // notification runs in this mode. The handler owns validation, safe scheduling,
    // capture, recovery and any history policy. Callbacks must not throw, and
    // rejected Begin must leave no transaction open. It must synchronously settle
    // Cancel before a rebind/cleanup; false keeps the session open for recovery.
    using EditIntentHandler = std::function<bool(EditPhase, uint64_t,
        const MaterialEditTargets&, const MaterialEditIntent&)>;
    void onEditRequested(EditIntentHandler handler);
    void cancelInteraction();

    bool draw(UserInterface& ui);
    bool queuePatch(const utils::MaterialModifier& modifier, bool ordinaryAo = false);
    bool queueIndirectMode(base::MaterialAttributes::IndirectTextureMode mode);
    bool queueTextureSettings(draw::TextureSlot slot, std::function<void(base::Texture&)> edit);
    bool queueTextureSettings(draw::TextureSlot slot, const TextureSamplingEdit& edit);
    bool queueClearTexture(draw::TextureSlot slot);

    bool pending() const;
    const std::string& error() const;
    // Runs after edits at prepareFrame, including partial failure. Value edits
    // may run without a GPU drain when explicitly enabled. The default deferred
    // upscaler reset is automatic; apps reset other histories here.
    void onChanged(std::function<void(draw::Renderer&)> callback);

  private:
    struct State;
    struct Impl;
    std::shared_ptr<State> _state;
    std::unique_ptr<Impl> _impl;

    bool requestEdit(MaterialEditIntent intent);

    bool queueEdit(std::function<void(draw::Drawable*, draw::MaterialBase*, uint32_t)> operation,
        bool cpuValuesOnly = false);
};

}
