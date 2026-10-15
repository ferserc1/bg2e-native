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
#include <bg2e/draw/DataBinding.hpp>
#include <bg2e/draw/ShaderData.hpp>
#include <bg2e/draw/environment/EnvironmentResources.hpp>

namespace bg2e::draw {
class BG2E_API EnvironmentDataBinding : public DataBinding {
  public:
    void initialize(Engine& engine, std::shared_ptr<gpu::PipelineLayout> layout, uint32_t frameSlots = 0, uint32_t metalTextureBase = 6);
    void update(const FrameContext& frame, std::shared_ptr<const environment::EnvironmentResources> environment);
    void cleanup() override;
  private:
    uint32_t _metalTextureBase = 6;
    std::vector<std::shared_ptr<const environment::EnvironmentResources>> _owners;
};
}
