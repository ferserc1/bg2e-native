#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/DescriptorSet.hpp>
#include <bg2e/render/Engine.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

class DescriptorSet;

class BG2E_API DescriptorSetAllocator {
public:
    ~DescriptorSetAllocator();
    
    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

    void init(Engine * engine);
    
    void requirePoolSizeRatio(
        uint32_t maxSets,
        const std::vector<PoolSizeRatio>& poolRatios
    );
    
    void initPool();

    void initPool(
        uint32_t maxSets,
        std::vector<PoolSizeRatio> poolRatios
    );

    void clearDescriptors();

    void destroy();

    VkDescriptorSet allocateRaw(VkDescriptorSetLayout layout, void* pNext = nullptr);
    
    DescriptorSet * allocate(VkDescriptorSetLayout layout, void* pNext = nullptr);

protected:
    uint32_t _initMaxSets = 0;
    std::vector<PoolSizeRatio> _initPoolRatios;
    bool _initialized = false;
    
    std::vector<VkDescriptorPoolSize> _poolSizes;
    std::vector<VkDescriptorPool> _fullPools;
    std::vector<VkDescriptorPool> _readyPools;
    uint32_t _setsPerPool;
    
    VkDescriptorPool getPool();
    VkDescriptorPool createPool(uint32_t setCount);

    Engine * _engine;
    
};

}
}
}

