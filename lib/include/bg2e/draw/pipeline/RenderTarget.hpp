#pragma once

#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>

namespace bg2e::draw::pipeline {
// Single-sample sampled attachment. Retain returned images through completion.
class BG2E_API RenderTarget {
  public:
    static std::shared_ptr<gpu::Image> create(Engine& engine, gpu::Size2D extent,
        gpu::PixelFormat format, const std::string& name = {});
    static void clear(gpu::CommandBuffer& command, gpu::Image& color, gpu::Color value = {0, 0, 0, 1});
    static void copy(gpu::CommandBuffer& command, gpu::Image& source, gpu::Image& destination);
};
}
