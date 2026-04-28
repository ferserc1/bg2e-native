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

#include <bg2e/app/OffscreenApplication.hpp>
#include <bg2e/render/vulkan/all.hpp>

#include <chrono>

namespace bg2e::app {

void OffscreenApplication::init(
    int argc, char ** argv,
    const std::string& appId,
    std::shared_ptr<OffscreenApplicationDelegate> delegate
) {
    _delegate = delegate;

    _delegate->initConfig(argc, argv, _config);

    _engine = std::make_unique<render::Engine>();
    _engine->init();

    _delegate->init(_engine.get());
    _engine->iterateFrameResources([&](render::vulkan::FrameResources& frameResources)
    {
        frameResources.descriptorAllocator->init(_engine.get());
        _delegate->initFrameResources(frameResources.descriptorAllocator);
    });
    _engine->descriptorSetAllocator().initPool();
    _delegate->initScene();
}

int OffscreenApplication::run()
{
    if (!_engine)
    {
        throw new std::runtime_error("OffscreenApplication::run(): the application is not initialized");
    }

    VkDevice device = _engine->device().handle();
    auto graphicsQueue = _engine->device().graphicsQueue();
    auto commandPool = _engine->command().createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    auto commandBuffer = _engine->command().allocateCommandBuffer(commandPool, 1);
    auto frameResources = _engine->currentFrameResources();

    _engine->cleanupManager().push([&](VkDevice dev)
    {
        vkDestroyCommandPool(dev, commandPool, nullptr);
    });
    auto frameFence = frameResources.frameFence;

    _delegate->resize(_config.width, _config.height);

    bool renderFrame = true;
    double elapsedMs = 0.0;
    uint32_t frameIndex = 0;

    while (renderFrame)
    {
        using namespace render::vulkan;

        VK_ASSERT(vkResetFences(device, 1, &frameFence));

        frameResources.flushFrameData();

        _delegate->frame(static_cast<float>(elapsedMs), frameIndex, frameResources);

        auto cmdBeginInfo = Info::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo));

        renderFrame = _delegate->render(commandBuffer, frameIndex, frameResources);

        VK_ASSERT(vkEndCommandBuffer(commandBuffer));

        auto cmdInfo = Info::commandBufferSubmitInfo(commandBuffer);
        auto submitInfo = Info::submitInfo(&cmdInfo, nullptr, nullptr);

        auto startTime = std::chrono::high_resolution_clock::now();
        VK_ASSERT(queueSubmit2(graphicsQueue, 1, &submitInfo, frameFence));
        VK_ASSERT(vkWaitForFences(device, 1, &frameFence, true, 100000000000));
        auto endTime = std::chrono::high_resolution_clock::now();

        elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        _delegate->didRenderFrame(frameIndex, elapsedMs);

        ++frameIndex;
        _engine->nextFrame();
    }

    cleanup();

    return 0;
}

void OffscreenApplication::cleanup()
{
    _delegate->cleanup();
    _engine->cleanup();
}

}
