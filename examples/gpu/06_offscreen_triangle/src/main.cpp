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
#include <bg2e/gpu/all.hpp>
#include <bg2e/base/all.hpp>
#include <bg2e/db/image.hpp>
#include <filesystem>
#include <vector>
#include <iostream>

struct PushConstants {
    float color[4];
};

int main(int argc, char** argv)
{
    using namespace bg2e;

    // 1. Select backend per platform
    auto backendType = gpu::BackendType::Vulkan;
    if (base::PlatformTools::currentPlatform() == base::Platform::macOS)
    {
        std::cout << "Select backend [1=Metal, 2=Vulkan]: ";
        int choice = 0;
        std::cin >> choice;
        backendType = (choice == 2) ? gpu::BackendType::Vulkan : gpu::BackendType::Metal;
    }

    // 2. Init backend
    gpu::Factory::init(backendType);
    auto* backend = gpu::Factory::backend();

    // 3. Create headless GPU instance (no SDL window)
    auto* instance = backend->sharedInstance();
    instance->enableDebugMode(true);
    instance->create();

    // 4. Create offscreen surface
    const gpu::Size2D size{ 800, 600 };
    std::shared_ptr<gpu::Surface> surface = backend->createOffscreenSurface(instance, size);

    // 5. Select physical device
    auto physicalDevice = backend->createPhysicalDevice();
    physicalDevice->choose(*instance, *surface);

    // 6. Create logical device
    auto device = backend->createDevice();
    device->create(instance, physicalDevice.get(), surface.get());

    // 7. Create shader modules and pipelines
    auto shaderBasePath = base::PlatformTools::shaderPath();
    std::string targetName = "gpu_offscreen_triangle";

    std::unique_ptr<gpu::ShaderModule> vs;
    std::unique_ptr<gpu::ShaderModule> fs;
    std::unique_ptr<gpu::ShaderModule> cs;

    if (backendType == gpu::BackendType::Vulkan)
    {
        auto vsPath = (shaderBasePath / targetName / "triangle.vert.spv").string();
        auto fsPath = (shaderBasePath / targetName / "triangle.frag.spv").string();
        auto csPath = (shaderBasePath / targetName / "noop.comp.spv").string();
        vs = device->createShaderModule({ vsPath, "main", gpu::ShaderStage::Vertex });
        fs = device->createShaderModule({ fsPath, "main", gpu::ShaderStage::Fragment });
        cs = device->createShaderModule({ csPath, "main", gpu::ShaderStage::Compute });
    }
    else
    {
        auto libPath = (shaderBasePath / targetName / "metal" / "triangle.metallib").string();
        vs = device->createShaderModule({ libPath, "triangle_vertex", gpu::ShaderStage::Vertex });
        fs = device->createShaderModule({ libPath, "triangle_fragment", gpu::ShaderStage::Fragment });
        cs = device->createShaderModule({ libPath, "noop_compute", gpu::ShaderStage::Compute });
    }

    // Graphics layout with push constant range for fragment stage
    gpu::PipelineLayoutDescription graphicsLayoutDesc{};
    graphicsLayoutDesc.pushConstants.push_back({ 0, sizeof(PushConstants), gpu::ShaderStage::Fragment });
    auto graphicsLayout = device->createPipelineLayout(graphicsLayoutDesc);

    // Compute layout (empty, no push constants or bindings)
    auto computeLayout = device->createPipelineLayout({});

    // Get attachment formats from surface
    auto colorFormat = surface->colorFormat();
    auto depthFormat = surface->depthFormat();

    gpu::GraphicsPipelineDescription pipelineDesc{};
    pipelineDesc.vertexShader = vs.get();
    pipelineDesc.fragmentShader = fs.get();
    pipelineDesc.layout = graphicsLayout.get();
    pipelineDesc.topology = gpu::PrimitiveTopology::TriangleList;
    pipelineDesc.colorFormat = colorFormat;
    pipelineDesc.depthFormat = depthFormat;

    auto pipeline = device->createGraphicsPipeline(pipelineDesc);

    // Compute pipeline
    gpu::ComputePipelineDescription computePipelineDesc{};
    computePipelineDesc.computeShader = cs.get();
    computePipelineDesc.layout = computeLayout.get();
    auto computePipeline = device->createComputePipeline(computePipelineDesc);

    // 8. Single render iteration (no loop — headless offscreen)
    auto& graphicsQueue = device->graphicsQueue();

    PushConstants push{};
    push.color[0] = 1.0f; push.color[1] = 0.4f; push.color[2] = 0.1f; push.color[3] = 1.0f;
    gpu::Color clearColor{ 0.1f, 0.1f, 0.15f, 1.0f };

    auto frame = surface->beginFrame();
    auto cmd   = graphicsQueue.createCommandBuffer();

    cmd->begin();

    // Compute dispatch (exercises the compute path offscreen)
    cmd->beginCompute();
    cmd->bindPipeline(computePipeline.get());
    cmd->dispatch(1, 1, 1);
    cmd->endCompute();

    // Graphics rendering
    cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
    cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
    cmd->beginRendering(frame.get());
    cmd->clearColor(0, clearColor);
    cmd->clearDepth(1.0f);
    cmd->bindPipeline(pipeline.get());
    cmd->pushConstants(gpu::ShaderStage::Fragment, 0, sizeof(PushConstants), &push);
    cmd->draw(3);
    cmd->endRendering();
    // No transition to Present — offscreen surface leaves color in ColorAttachment

    surface->present(cmd.get());
    cmd->end();
    graphicsQueue.submit(cmd.get());
    surface->endFrame(frame.get());

    // Wait for GPU to finish — offscreen submit has no fence
    device->waitIdle();

    // 9. Read back pixels and save to disk
    const uint32_t w = surface->width();
    const uint32_t h = surface->height();
    std::vector<uint8_t> pixels;
    frame->colorImage()->readPixelsRGBA8(pixels, gpu::ImageLayout::ColorAttachment);

    auto outPath = std::filesystem::current_path() / "out.jpg";
    bg2e::db::saveImage(outPath, pixels.data(), w, h, 4);
    std::cout << "Wrote " << outPath << " (" << w << "x" << h << ")\n";

    // 10. Cleanup
    computePipeline->cleanup();
    pipeline->cleanup();
    computeLayout->cleanup();
    graphicsLayout->cleanup();
    cs->cleanup();
    vs->cleanup();
    fs->cleanup();
    surface->cleanup();
    device->cleanup();
    instance->cleanup();

    return 0;
}
