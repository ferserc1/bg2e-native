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
#include <string>

namespace bg2e::gpu::metal {
// Native temporal reconstruction boundary. No draw/scene or Objective-C types.
// One scaler per ordered graphics stream. Initialize/cleanup require a drained
// device; borrowed dispatch images and per-slot resources survive GPU completion.
struct MetalFXDispatch {
    gpu::CommandBuffer& command;
    gpu::Image& color;
    gpu::Image& depth; // R32F, zero-to-one, conventional near=0/far=1.
    gpu::Image& motion; // RG16F, unjittered previous-current input pixels, Y down.
    gpu::Image& output; // Linear HDR RGBA16F; reconstruction precedes tone mapping.
    uint32_t slot = 0;
    float jitterX = 0, jitterY = 0;
    bool reset = true;
};
class BG2E_API MetalFXBridge {
  public:
    MetalFXBridge();
    ~MetalFXBridge();
    MetalFXBridge(const MetalFXBridge&) = delete;
    MetalFXBridge& operator=(const MetalFXBridge&) = delete;

    static bool sdkAvailable();
    static bool supportsDevice(const gpu::Device& device);
    bool initialize(gpu::Device& device, Size2D internal, Size2D output, uint32_t frameSlots);
    void cleanup();

    bool available() const;
    const std::string& status() const;
    bool record(const MetalFXDispatch& dispatch);

  private:
    struct State;
    std::unique_ptr<State> _state;
    std::string _status = "Not initialized";
};
}
