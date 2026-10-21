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

#include <bg2e/bake/StandaloneLightmapBaker.hpp>

namespace bg2e::draw { class Engine; }
namespace bg2e::bake {
// Borrows an initialized windowless Engine. Owns a serial UV-job lifecycle,
// independent of camera rendering, UI, presentation slots and display histories.
// Stop all other scene/resource producers through preparation, updates and cleanup.
// Engine/Device outlive context, bakers, targets and exported GPU owners.
class BG2E_API StandaloneBakerContext final : public BakerContext {
  public:
    enum class State
    {
        Created, Initialized, SceneReady, Cleaned
    };

    explicit StandaloneBakerContext(draw::Engine& engine);
    ~StandaloneBakerContext() override;

    State state() const;
    const BakerContextConfiguration& configuration() const override;
    uint64_t sourceGeneration() const override;

    // One private serial slot, regardless of the Engine surface slot count.
    void initialize(std::filesystem::path shaderRoot = {}, BakePassConfiguration passes = {});
    // Retains an already loaded root; caller performs any scene update/UV2 first.
    // Null environment generates the selected authored/default sky via public tools.
    // Every attempt invalidates old bakers; failure leaves Initialized, not ready.
    void prepareScene(std::shared_ptr<scene::Node> root,
        std::shared_ptr<const draw::environment::EnvironmentResources> environment = nullptr);
    std::shared_ptr<StandaloneLightmapBaker> createBaker(LightmapBakerConfiguration configuration);

    void invalidateSources() override; // Requires a new prepareScene and new baker.
    void cleanup() override; // Idempotent; does not clean the borrowed Engine.

  private:
    std::shared_ptr<StandaloneBakeState> _state;
};
}
