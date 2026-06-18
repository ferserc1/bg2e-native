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

#include <bg2e/ui/UserInterface.hpp>
#include <bg2e/render/vulkan/Command.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/common.hpp>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include "bg2e/app/Preferences.hpp"
#include "bg2e/app/PreferencesStore.hpp"

namespace bg2e {
namespace ui {

float UserInterface::s_uiScale = 1.0f;
bool UserInterface::s_uiFontLoaded = false;
bool UserInterface::s_uiScaleChanged = true;

static bool s_baseStyleInitialized = false;
static ImGuiStyle s_baseStyle;

void UserInterface::init(render::Engine * engine)
{
    auto preferences = bg2e::app::PreferencesStore::instance().preferences("ui");
    s_uiScale = preferences.get("uiScale", s_uiScale);

    _engine = engine;

    initCommands();

    auto fenceInfo = render::vulkan::Info::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_ASSERT(vkCreateFence(_engine->device().handle(), &fenceInfo, nullptr, &_uiFence));

    initImGui();
    
    if (_delegate)
    {
        _delegate->init(engine, this);
    }
}

void UserInterface::setScale(float scale)
{
    s_uiScale = scale;
    s_uiScaleChanged = true;
}

void UserInterface::updateScale()
{
    if (!s_baseStyleInitialized)
    {
        s_baseStyle = ImGui::GetStyle();

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        auto assets = base::PlatformTools::assetPath() / "DidactGothic-Regular.ttf";
        io.Fonts->AddFontFromFileTTF(assets.string().c_str(), 16.0f);
        io.Fonts->Build();
        s_uiFontLoaded = true;
        s_baseStyleInitialized = true;
    }

    auto & style = ImGui::GetStyle();
    style = s_baseStyle;

    style.ScaleAllSizes(s_uiScale);
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = s_uiScale;
    s_uiScaleChanged = false;
}

void UserInterface::processEvent(SDL_Event* event)
{
    ImGui_ImplSDL2_ProcessEvent(event);
}

void UserInterface::setFrameOverride(std::function<void()> fn)
{
    _frameOverride = std::move(fn);
}

void UserInterface::clearFrameOverride()
{
    _frameOverride = nullptr;
}

void UserInterface::newFrame()
{
    if (s_uiScaleChanged)
    {
        updateScale();
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (_frameOverride)
    {
        _frameOverride();
    }
    else if (_delegate) {
        _delegate->drawUI();
    }

    ImGui::Render();
}

void UserInterface::draw(VkCommandBuffer cmd, VkImageView targetImageView)
{
    using namespace bg2e::render::vulkan;
    auto colorAttachment = Info::attachmentInfo(
        targetImageView,
        nullptr,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    auto extent = _engine->swapchain().extent();
    auto renderingInfo = Info::renderingInfo(
        extent,
        &colorAttachment,
        nullptr
    );

    cmdBeginRendering(cmd, &renderingInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    cmdEndRendering(cmd);
}

void UserInterface::cleanup()
{
    auto preferences = bg2e::app::PreferencesStore::instance().preferences("ui");
    preferences.set("uiScale", s_uiScale);
}

void UserInterface::initCommands()
{
    _commandPool = _engine->command().createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    _commandBuffer = _engine->command().allocateCommandBuffer(_commandPool, 1);

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyFence(dev, _uiFence, nullptr);
        _engine->command().destroyComandPool(_commandPool);
    });
}

void UserInterface::initImGui()
{
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = uint32_t(std::size(poolSizes));
    poolInfo.pPoolSizes = poolSizes;

    VK_ASSERT(vkCreateDescriptorPool(_engine->device().handle(), &poolInfo, nullptr, &_imguiPool));

    ImGui::CreateContext();

    SDL_Window* window = reinterpret_cast<SDL_Window*>(_engine->windowPtr());
    ImGui_ImplSDL2_InitForVulkan(window);

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = static_cast<gpu::vk::Instance*>(_engine->instance())->vkInstanceHnd();
    initInfo.PhysicalDevice = _engine->physicalDevice().handle();
    initInfo.Device = _engine->device().handle();
    initInfo.Queue = _engine->command().graphicsQueue();
    initInfo.DescriptorPool = _imguiPool;
    initInfo.MinImageCount = _engine->numImages();
    initInfo.ImageCount = _engine->numImages();
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = {};
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    VkFormat format = _engine->swapchain().imageFormat();
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo);

    _engine->cleanupManager().push([&, this](VkDevice dev) {
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(dev, _imguiPool, nullptr);
    });
}

}
}
