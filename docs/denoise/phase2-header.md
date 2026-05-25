# Fase 2: Header de la Clase DenoiseFilter

## Archivo

`lib/include/bg2e/render/deferred/DenoiseFilter.hpp`

## Estructura

```cpp
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/gbuffer/GBufferManager.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/Sampler.hpp>

#include <vector>
#include <memory>

namespace bg2e {
namespace render {
namespace deferred {

class BG2E_API DenoiseFilter {
public:
    DenoiseFilter(Engine* engine);
    ~DenoiseFilter();

    void build(const GBufferManager* gbuffer, VkExtent2D extent);
    void resize(VkExtent2D newExtent);
    void render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        vulkan::FrameResources& frameResources,
        const vulkan::Image* inputImage
    );
    void cleanup();

    std::shared_ptr<vulkan::Image> outputImage(uint32_t frameIndex) const;
    VkSampler sampler() const;

    // Accessors para parámetros en tiempo real
    void setKernelRadius(int radius);
    void setDepthThreshold(float threshold);
    void setNormalThreshold(float threshold);
    void setDepthSigma(float sigma);
    void setNormalSigma(float sigma);

    int kernelRadius() const;
    float depthThreshold() const;
    float normalThreshold() const;
    float depthSigma() const;
    float normalSigma() const;

private:
    Engine* _engine;
    VkExtent2D _extent;

    // Imágenes de resultado por-frame
    std::vector<std::shared_ptr<vulkan::Image>> _outputImages;

    // Vulkan objects
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _dsLayout = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;

    // Parámetros del filtro
    int _kernelRadius = 3;
    float _depthThreshold = 0.01f;
    float _normalThreshold = 0.8f;
    float _depthSigma = 0.01f;
    float _normalSigma = 0.3f;

    // Push constants (debe coincidir con el shader)
    struct DenoisePushConstants {
        glm::vec2 outputSize;
        int kernelRadius;
        float depthThreshold;
        float normalThreshold;
        float depthSigma;
        float normalSigma;
        uint32_t padding;  // align to 16 bytes
    };

    void createOutputImages(VkExtent2D extent);
    void createPipeline();
    void cleanupImages();
};

}
}
}
```

## Notas

- El namespace es `bg2e::render::deferred` (igual que RTAmbientOcclusion)
- `BG2E_API` macro para exportar la clase desde la DLL/shared lib
- Imágenes `VK_FORMAT_R8_UNORM` (una sola componente, igual que AO)
- Usage flags: `VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT`
- `createOutputImages()` se llama desde `build()` y `resize()`
- `cleanupImages()` limpia las imágenes sin destruir los pipelines (se hace en `cleanup()`)
- Los accessors permiten modificar parámetros en tiempo real sin recompilar el pipeline