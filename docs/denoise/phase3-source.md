# Fase 3: Implementación de DenoiseFilter

## Archivo

`lib/src/bg2e/render/deferred/DenoiseFilter.cpp`

## Descripción

Implementación completa de la clase DenoiseFilter siguiendo el patrón de RTAmbientOcclusion.

## Funciones

### Constructor / Destructor

```cpp
DenoiseFilter::DenoiseFilter(Engine* engine)
    : _engine{engine}
{
}

DenoiseFilter::~DenoiseFilter()
{
    cleanup();
}
```

### build()

```cpp
void DenoiseFilter::build(const GBufferManager* gbuffer, VkExtent2D extent)
{
    _extent = extent;

    // Crear sampler para muestrear imágenes de entrada (sampler2D)
    vulkan::factory::Sampler samplerFactory(_engine);
    _sampler = samplerFactory.build();

    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroySampler(dev, _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    });

    // Crear imágenes de resultado por-frame
    createOutputImages(extent);

    // Crear pipeline compute
    createPipeline();
}
```

### createOutputImages()

```cpp
void DenoiseFilter::createOutputImages(VkExtent2D extent)
{
    cleanupImages();

    _outputImages.resize(_engine->numImages());
    for (uint32_t i = 0; i < _outputImages.size(); i++)
    {
        _outputImages[i] = std::shared_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _engine,
                "Denoise output image " + std::to_string(i),
                VK_FORMAT_R8_UNORM,
                extent,
                VK_IMAGE_USAGE_STORAGE_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT,
                1, false, 0, VK_SAMPLE_COUNT_1_BIT
            )
        );
    }
}
```

### createPipeline()

```cpp
void DenoiseFilter::createPipeline()
{
    // Descriptor set layout: 4 bindings
    vulkan::factory::DescriptorSetLayout dsLayoutFactory;
    dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // input image
    dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_Normal
    dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);  // g_Depth
    dsLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);            // output
    _dsLayout = dsLayoutFactory.build(
        _engine->device().handle(),
        VK_SHADER_STAGE_COMPUTE_BIT
    );

    // Pipeline layout: descriptor set + push constants
    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addDescriptorSetLayout(_dsLayout);
    layoutFactory.addPushConstantRange(
        0,
        sizeof(DenoisePushConstants),
        VK_SHADER_STAGE_COMPUTE_BIT
    );
    _pipelineLayout = layoutFactory.build();

    // Compute pipeline
    vulkan::factory::ComputePipeline plFactory(_engine);
    plFactory.setShader("denoise_bilateral.comp.spv");
    _pipeline = plFactory.build(_pipelineLayout);

    // Cleanup registration
    _engine->cleanupManager().push([&](VkDevice dev) {
        vkDestroyPipeline(dev, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(dev, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
        vkDestroyDescriptorSetLayout(dev, _dsLayout, nullptr);
        _dsLayout = VK_NULL_HANDLE;
    });
}
```

### render()

```cpp
void DenoiseFilter::render(
    VkCommandBuffer cmd,
    uint32_t currentFrame,
    vulkan::FrameResources& frameResources,
    const vulkan::Image* inputImage
)
{
    auto outputImage = _outputImages[_engine->currentFrameResourcesIndex()];

    // 1. Transicionar output a GENERAL (write layout)
    vulkan::Image::cmdTransitionImage(cmd, outputImage->handle(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // 2. Asignar descriptor set desde frameResources
    auto ds = frameResources.newDescriptorSet(_dsLayout);
    ds->beginUpdate();
    ds->addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        inputImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->image(1).get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        gbuffer->depthImage().get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _sampler);
    ds->addImage(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        outputImage.get(), VK_IMAGE_LAYOUT_GENERAL);
    ds->endUpdate();

    // 3. Bind pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);

    // 4. Bind descriptor sets
    VkDescriptorSet dsHandle = ds->descriptorSet();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        _pipelineLayout, 0, 1, &dsHandle, 0, nullptr);

    // 5. Push constants
    DenoisePushConstants pc{};
    pc.outputSize = glm::vec2(static_cast<float>(_extent.width), static_cast<float>(_extent.height));
    pc.kernelRadius = _kernelRadius;
    pc.depthThreshold = _depthThreshold;
    pc.normalThreshold = _normalThreshold;
    pc.depthSigma = _depthSigma;
    pc.normalSigma = _normalSigma;
    vkCmdPushConstants(cmd, _pipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(DenoisePushConstants), &pc);

    // 6. Dispatch
    uint32_t groupX = static_cast<uint32_t>(std::ceil(_extent.width / 8.0f));
    uint32_t groupY = static_cast<uint32_t>(std::ceil(_extent.height / 8.0f));
    vkCmdDispatch(cmd, groupX, groupY, 1);

    // 7. Transicionar output a SHADER_READ_ONLY_OPTIMAL (para el composite pass)
    vulkan::Image::cmdTransitionImage(cmd, outputImage->handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
```

**Nota importante sobre render()**: La función `render()` necesita acceso al `GBufferManager` para obtener las normales y profundidad. Hay dos opciones:

1. **Opción A**: El método `render()` recibe también un pointer al GBufferManager:
   ```cpp
   void render(VkCommandBuffer cmd, uint32_t currentFrame,
               vulkan::FrameResources& frameResources,
               const GBufferManager* gbuffer,
               const vulkan::Image* inputImage);
   ```

2. **Opción B**: El `GBufferManager` se almacena como miembro en `build()` y se reutiliza en `render()`.

Se recomienda la **Opción A** porque el GBufferManager puede cambiar entre capas (opaque vs transparent), y es más flexible.

### resize()

```cpp
void DenoiseFilter::resize(VkExtent2D newExtent)
{
    _extent = newExtent;
    createOutputImages(newExtent);
}
```

### cleanup()

```cpp
void DenoiseFilter::cleanup()
{
    cleanupImages();

    if (_pipeline)
    {
        vkDestroyPipeline(_engine->device().handle(), _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
    }
    if (_pipelineLayout)
    {
        vkDestroyPipelineLayout(_engine->device().handle(), _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
    }
    if (_dsLayout)
    {
        vkDestroyDescriptorSetLayout(_engine->device().handle(), _dsLayout, nullptr);
        _dsLayout = VK_NULL_HANDLE;
    }
    if (_sampler)
    {
        vkDestroySampler(_engine->device().handle(), _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    }
}
```

### cleanupImages()

```cpp
void DenoiseFilter::cleanupImages()
{
    for (auto& img : _outputImages)
    {
        if (img) img->cleanup();
    }
    _outputImages.clear();
}
```

### Accessors

```cpp
void DenoiseFilter::setKernelRadius(int radius) { _kernelRadius = radius; }
void DenoiseFilter::setDepthThreshold(float threshold) { _depthThreshold = threshold; }
void DenoiseFilter::setNormalThreshold(float threshold) { _normalThreshold = threshold; }
void DenoiseFilter::setDepthSigma(float sigma) { _depthSigma = sigma; }
void DenoiseFilter::setNormalSigma(float sigma) { _normalSigma = sigma; }

int DenoiseFilter::kernelRadius() const { return _kernelRadius; }
float DenoiseFilter::depthThreshold() const { return _depthThreshold; }
float DenoiseFilter::normalThreshold() const { return _normalThreshold; }
float DenoiseFilter::depthSigma() const { return _depthSigma; }
float DenoiseFilter::normalSigma() const { return _normalSigma; }
```

### outputImage()

```cpp
std::shared_ptr<vulkan::Image> DenoiseFilter::outputImage(uint32_t frameIndex) const
{
    return _outputImages[frameIndex];
}
```

### sampler()

```cpp
VkSampler DenoiseFilter::sampler() const
{
    return _sampler;
}
```

## Dependencias

- `#include <bg2e/render/deferred/DenoiseFilter.hpp>`
- `#include <bg2e/render/vulkan/factory/ComputePipeline.hpp>`
- `#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>`
- `#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>`
- `#include <bg2e/render/vulkan/factory/Sampler.hpp>`
- `#include <cmath>` (para `std::ceil`)