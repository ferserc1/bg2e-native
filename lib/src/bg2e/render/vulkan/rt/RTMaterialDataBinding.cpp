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

#include <bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/Texture.hpp>

namespace bg2e {
namespace render {
namespace vulkan {
namespace rt {

RTMaterialDataBinding::RTMaterialDataBinding(Engine * engine)
    : PipelineDataBinding(engine)
{
}

void RTMaterialDataBinding::initFrameResources(DescriptorSetAllocator * frameAllocator)
{
    frameAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 + 2 * MAX_OBJECTS },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * MAX_OBJECTS }
    });
}

VkDescriptorSetLayout RTMaterialDataBinding::createLayout(VkShaderStageFlags shaderStages)
{
    if (_layout == VK_NULL_HANDLE)
    {
        factory::DescriptorSetLayout dsLayoutFactory;
        dsLayoutFactory.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        dsLayoutFactory.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_OBJECTS);
        dsLayoutFactory.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_OBJECTS);
        dsLayoutFactory.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_OBJECTS);
        dsLayoutFactory.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_OBJECTS);
        _layout = dsLayoutFactory.build(_engine->device().handle(), shaderStages);
    }
    return _layout;
}

VkDescriptorSet RTMaterialDataBinding::newDescriptorSet(
    FrameResources & frameResources,
    const std::vector<RTObjectInstance> & objectInstances
) {
    size_t instanceCount = std::min<size_t>(objectInstances.size(), MAX_OBJECTS);
    if (_layout == VK_NULL_HANDLE)
    {
        throw std::runtime_error("RTMaterialDataBinding::newDescriptorSet() - Layout not created");
    }

    std::vector<RTMaterialData> materialData;
    materialData.reserve(objectInstances.size());
    for (const auto & inst : objectInstances)
    {
        materialData.push_back(inst.materialData);
    }

    if (materialData.empty())
    {
        materialData.push_back(RTMaterialData{});
    }

    auto * materialBuffer = macros::createBuffer(
        _engine, frameResources, materialData,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        "RTMaterialDataBinding: material data SSBO"
    );

    auto ds = frameResources.newDescriptorSet(_layout);

    ds->beginUpdate();

    ds->addBuffer(
        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        materialBuffer, materialData.size() * sizeof(RTMaterialData), 0
    );

    auto whiteTex = Texture::whiteTexture(_engine);

    {
        std::vector<VkBuffer> vertexBuffers(instanceCount, VK_NULL_HANDLE);
        std::vector<size_t> bufferSizes(instanceCount, 0);
        std::vector<size_t> bufferOffsets(instanceCount, 0);

        for (uint32_t i = 0; i < instanceCount; ++i)
        {
            if (i < objectInstances.size() && objectInstances[i].vertexBuffer)
            {
                vertexBuffers[i] = objectInstances[i].vertexBuffer->handle();
                bufferSizes[i] = static_cast<size_t>(objectInstances[i].vertexBuffer->logicSize());
            }
            else
            {
                if (_dummyBuffer == VK_NULL_HANDLE)
                {
                    _dummyBufferObj = Buffer::createAllocatedBuffer(
                        _engine, 1,
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                        VMA_MEMORY_USAGE_CPU_ONLY,
                        "RTMaterialDataBinding: dummy buffer"
                    );
                    _dummyBuffer = _dummyBufferObj->handle();
                }
                vertexBuffers[i] = _dummyBuffer;
                bufferSizes[i] = 0;
            }
        }

        ds->addBufferArray(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, vertexBuffers, bufferSizes, bufferOffsets);
    }

    {
        std::vector<VkBuffer> indexBuffers(instanceCount, VK_NULL_HANDLE);
        std::vector<size_t> bufferSizes(instanceCount, 0);
        std::vector<size_t> bufferOffsets(instanceCount, 0);

        for (uint32_t i = 0; i < instanceCount; ++i)
        {
            if (i < objectInstances.size() && objectInstances[i].indexBuffer)
            {
                indexBuffers[i] = objectInstances[i].indexBuffer->handle();
                bufferSizes[i] = static_cast<size_t>(objectInstances[i].indexBuffer->logicSize());
            }
            else
            {
                if (_dummyBuffer == VK_NULL_HANDLE)
                {
                    _dummyBufferObj = Buffer::createAllocatedBuffer(
                        _engine, 1,
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                        VMA_MEMORY_USAGE_CPU_ONLY,
                        "RTMaterialDataBinding: dummy buffer"
                    );
                    _dummyBuffer = _dummyBufferObj->handle();
                }
                indexBuffers[i] = _dummyBuffer;
                bufferSizes[i] = 0;
            }
        }

        ds->addBufferArray(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, indexBuffers, bufferSizes, bufferOffsets);
    }

    {
        std::vector<VkImageView> imageViews(instanceCount);
        std::vector<VkSampler> samplers(instanceCount);

        for (uint32_t i = 0; i < instanceCount; ++i)
        {
            if (i < objectInstances.size() && objectInstances[i].albedoTexture)
            {
                imageViews[i] = objectInstances[i].albedoTexture->image()->imageView();
                samplers[i] = objectInstances[i].albedoTexture->sampler();
            }
            else
            {
                imageViews[i] = whiteTex->image()->imageView();
                samplers[i] = whiteTex->sampler();
            }
        }

        ds->addImageArray(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageViews, samplers);
    }

    {
        std::vector<VkImageView> imageViews(instanceCount);
        std::vector<VkSampler> samplers(instanceCount);

        for (uint32_t i = 0; i < instanceCount; ++i)
        {
            if (i < objectInstances.size() && objectInstances[i].lightEmissionTexture)
            {
                imageViews[i] = objectInstances[i].lightEmissionTexture->image()->imageView();
                samplers[i] = objectInstances[i].lightEmissionTexture->sampler();
            }
            else
            {
                imageViews[i] = whiteTex->image()->imageView();
                samplers[i] = whiteTex->sampler();
            }
        }

        ds->addImageArray(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageViews, samplers);
    }

    ds->endUpdate();
    return ds->descriptorSet();
}

void RTMaterialDataBinding::cleanup()
{
    PipelineDataBinding::cleanup();

    if (_dummyBuffer != VK_NULL_HANDLE)
    {
        _engine->destroyBuffer(_dummyBuffer, _dummyBufferObj->allocation());
        _dummyBuffer = VK_NULL_HANDLE;
        delete _dummyBufferObj;
        _dummyBufferObj = nullptr;
    }
}

}
}
}
}