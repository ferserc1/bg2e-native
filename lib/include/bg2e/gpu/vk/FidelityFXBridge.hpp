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

#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/Image.hpp>
#include <memory>

namespace bg2e::gpu::vk {
// Narrow native SDK boundary. No SDK/Vulkan types escape this header.
// One context per sequential graphics submission stream. Caller drains GPU
// before resize/cleanup and retains all borrowed dispatch images until completion.
struct FidelityFXDispatch {
    gpu::CommandBuffer& command;
    gpu::Image& color;
    gpu::Image& depth;
    gpu::Image& motion;
    gpu::Image& output;
    float jitterX = 0, jitterY = 0;
    float deltaMilliseconds = 16.67f;
    float nearPlane = 0.1f, farPlane = 1000, verticalFov = 1.04719755f;
    float sharpness = 0.5f;
    bool reset = true;
};
class BG2E_API FidelityFXBridge {
  public:
    FidelityFXBridge();
    ~FidelityFXBridge();
    FidelityFXBridge(const FidelityFXBridge&) = delete;
    FidelityFXBridge& operator=(const FidelityFXBridge&) = delete;
    static bool sdkAvailable();
    bool initialize(gpu::Device& device, Size2D internal, Size2D output, uint32_t frameSlots);
    bool available() const;
    const std::string& status() const;
    const std::string& hardwareStatus() const;
    void jitter(uint64_t frame, float& x, float& y) const;
    bool record(const FidelityFXDispatch& dispatch);
    void cleanup();
  private:
    struct State;
    std::unique_ptr<State> _state;
};
}
