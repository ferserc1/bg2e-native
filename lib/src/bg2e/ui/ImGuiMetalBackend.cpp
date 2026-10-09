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

#include <bg2e/base/PlatformTools.hpp>
#ifdef BG2E_IS_MAC
#include "detail/ImGuiBackend.hpp"
#include <bg2e/draw/Engine.hpp>
#include <bg2e/gpu/metal/Device.hpp>
#include <bg2e/gpu/metal/CommandBuffer.hpp>
#include <bg2e/gpu/metal/Image.hpp>
#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>
#include "imgui.h"
#include "imgui_impl_metal.h"


#include <stdexcept>

namespace bg2e::ui::detail {
namespace {
struct AutoreleaseScope {
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
    ~AutoreleaseScope() { pool->release(); }
};
class MetalImGuiBackend final : public ImGuiBackend {
public:
    void initialize(render::Engine&) override
    {
        throw std::invalid_argument("Production UI uses the Vulkan renderer.");
    }

    void initialize(draw::Engine& engine) override
    {
        auto* device = dynamic_cast<gpu::metal::Device*>(engine.device());
        if (!device || !device->isValid()) throw std::invalid_argument("Metal UI requires a valid Metal Device.");
        _engine = &engine;
        {
            AutoreleaseScope autoreleaseScope;
            _rendererStarted = true;
            if (!ImGui_ImplMetal_Init(device->handle()))
                throw std::runtime_error("ImGui Metal initialization failed.");
        }
    }

    void prepareFrame(gpu::CommandBuffer* command, gpu::SurfaceFrame* frame) override
    {
        if (!_engine || !_rendererStarted || !command || !frame || !frame->isValid())
            throw std::logic_error("Metal UI preparation requires an acquired presentation frame.");
        if (!dynamic_cast<gpu::metal::CommandBuffer*>(command) || command->hasActiveScope())
            throw std::invalid_argument("Metal UI preparation requires Metal commands without active scopes.");
        auto* image = dynamic_cast<gpu::metal::Image*>(frame->colorImage());
        if (!image || !image->texture()) throw std::invalid_argument("Metal UI requires a Metal presentation image.");
        auto* texture = image->texture();
        if (!_preparation || _generation != _engine->surface()->generation() ||
            _format != texture->pixelFormat() || _sampleCount != texture->sampleCount())
        {
            releasePreparation();
            _preparation = MTL::RenderPassDescriptor::alloc()->init();
            if (!_preparation) throw std::runtime_error("Could not allocate Metal UI preparation metadata.");
            _generation = _engine->surface()->generation();
            _format = texture->pixelFormat();
            _sampleCount = texture->sampleCount();
        }
        auto* color = _preparation->colorAttachments()->object(0);
        color->setTexture(texture);
        color->setLoadAction(MTL::LoadActionLoad);
        color->setStoreAction(MTL::StoreActionStore);
        // Metadata only: no encoder is opened while scene/copy work follows.
        {
            AutoreleaseScope autoreleaseScope;
            ImGui_ImplMetal_NewFrame(_preparation);
        }
    }

    void draw(VkCommandBuffer, VkImageView) override
    {
        throw std::invalid_argument("Metal UI does not accept native Vulkan commands.");
    }

    void draw(gpu::CommandBuffer& command, gpu::SurfaceFrame& frame) override
    {
        auto* metalCommand = dynamic_cast<gpu::metal::CommandBuffer*>(&command);
        if (!_engine || !_preparation || !metalCommand || !frame.isValid() || !frame.colorImage())
            throw std::invalid_argument("Metal UI composition requires a prepared Metal frame.");
        if (command.hasActiveScope()) throw std::logic_error("Metal UI composition requires closed scene scopes.");
        {
            AutoreleaseScope autoreleaseScope;
            command.beginRendering(frame.colorImage()); // Color-only LOAD/STORE.
            auto* pass = metalCommand->renderPassDescriptor();
            auto* texture = pass->colorAttachments()->object(0)->texture();
            if (!texture || texture->pixelFormat() != _format || texture->sampleCount() != _sampleCount ||
                pass->depthAttachment()->texture() || pass->stencilAttachment()->texture())
                throw std::logic_error("Metal UI preparation and overlay pass configurations differ.");
            auto* encoder = metalCommand->materializeRenderEncoder();
            ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(),
                metalCommand->handle(),
                encoder);
            command.endRendering(); // GPU owns and ends the encoder exactly once.
        }
    }

    void shutdown() override
    {
        {
            AutoreleaseScope autoreleaseScope;
            if (_rendererStarted && ImGui::GetCurrentContext() && ImGui::GetIO().BackendRendererUserData)
                ImGui_ImplMetal_Shutdown();
            _rendererStarted = false;
            releasePreparation();
            _engine = nullptr;
        }
    }

private:
    void releasePreparation()
    {
        if (_preparation) _preparation->release();
        _preparation = nullptr;
    }

    draw::Engine* _engine = nullptr;
    MTL::RenderPassDescriptor* _preparation = nullptr;
    MTL::PixelFormat _format = MTL::PixelFormatInvalid;
    NS::UInteger _sampleCount = 0;
    uint64_t _generation = 0;
    bool _rendererStarted = false;
};
}

std::unique_ptr<ImGuiBackend> createMetalImGuiBackend()
{
    return std::make_unique<MetalImGuiBackend>();
}
}
#endif
