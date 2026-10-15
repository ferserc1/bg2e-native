#include "probe.hpp"
#include "../subresources/probe.hpp"
#include "../shader_libraries/probe.hpp"
#include <bg2e/gpu/Factory.hpp>
#include <bg2e/gpu/Backend.hpp>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv)
{
    using namespace bg2e::gpu;
    if ((argc != 3 && argc != 4) || (std::string(argv[1]) != "vulkan" && std::string(argv[1]) != "metal"))
    {
        std::cerr << "Usage: mrt_probe vulkan|metal COMPILED_SHADER_DIRECTORY [ENGINE_SHADER_DIRECTORY]\n";
        return 2;
    }
    const bool metal = std::string(argv[1]) == "metal";
    const std::filesystem::path shaders(argv[2]);
    std::shared_ptr<Backend> backend;
    Instance* instance = nullptr;
    std::unique_ptr<OffscreenSurface> surface;
    std::unique_ptr<Device> device;
    try
    {
        backend = Factory::acquireBackend(metal ? BackendType::Metal : BackendType::Vulkan);
        instance = backend->sharedInstance();
        instance->enableDebugMode(true);
        instance->setApplicationName("Primitive MRT probe");
        instance->create();
        surface = backend->createOffscreenSurface(instance, {16, 16});
        auto physical = backend->createPhysicalDevice();
        physical->choose(*instance, *surface);
        device = backend->createDevice();
        device->create(instance, physical.get(), surface.get());
        {
            auto vertex = device->createShaderModule({
                .filePath = (shaders / (metal ? "probe.metallib" : "probe.vert.spv")).string(),
                .entryPoint = metal ? "probeVertex" : "main",
                .stage = ShaderStage::Vertex});
            auto fragment = device->createShaderModule({
                .filePath = (shaders / (metal ? "probe.metallib" : "probe.frag.spv")).string(),
                .entryPoint = metal ? "probeFragment" : "main",
                .stage = ShaderStage::Fragment});
            tests::runMrtProbe(*device, *vertex, *fragment);
            tests::runSubresourceProbe(*device);
            if (argc == 4)
            {
                tests::runShaderLibraryProbe(*device, metal ? BackendType::Metal : BackendType::Vulkan, argv[3]);
            }
        }
        surface->cleanup();
        surface.reset();
        device->cleanup();
        device.reset();
        instance->cleanup();
        std::cout << "MRT, depth and image subresource/mipmap probes passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        if (surface)
        {
            surface->cleanup();
            surface.reset();
        }
        if (device)
        {
            device->cleanup();
            device.reset();
        }
        if (instance)
        {
            instance->cleanup();
        }
        return 1;
    }
}
