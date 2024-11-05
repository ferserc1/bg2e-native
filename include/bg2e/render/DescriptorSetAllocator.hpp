#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/common.hpp>
#include <bg2e/render/Vulkan.hpp>
#include <bg2e/render/DescriptorSet.hpp>

namespace bg2e {
namespace render {

class DescriptorSet;

class BG2E_API DescriptorSetAllocator {
public:
    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

    void init(Vulkan * vulkan);

    void initPool(
        uint32_t maxSets,
        std::vector<PoolSizeRatio> poolRatios
    );

    void clearDescriptors();

    void destroy();

    VkDescriptorSet allocateRaw(VkDescriptorSetLayout layout, void* pNext = nullptr);
    
    DescriptorSet * allocate(VkDescriptorSetLayout layout, void* pNext = nullptr);

protected:
    std::vector<VkDescriptorPoolSize> _poolSizes;
    std::vector<VkDescriptorPool> _fullPools;
    std::vector<VkDescriptorPool> _readyPools;
    uint32_t _setsPerPool;
    
    VkDescriptorPool getPool();
    VkDescriptorPool createPool(uint32_t setCount);

    Vulkan * _vulkan;
    
};

}
}
