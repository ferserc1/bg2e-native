
#include <bg2e/base/Log.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

void DescriptorSetAllocator::init(Vulkan * vulkan)
{
    _vulkan = vulkan;
}

void DescriptorSetAllocator::requirePoolSizeRatio(
    uint32_t maxSets,
    const std::vector<PoolSizeRatio> &poolRatios
) {
    if (_initialized)
    {
        throw std::runtime_error("Trying to add pool size ratio requirement on an initialized Descriptor Set Allocator. The pool size ratios can't be modified once the allocator is inialized");
    }
    
    _initMaxSets = std::max(_initMaxSets, maxSets);
    for (auto & ratio : poolRatios)
    {
        _initPoolRatios.push_back(ratio);
    }
}

void DescriptorSetAllocator::initPool()
{
    if (_initPoolRatios.empty())
    {
        bg2e_log_debug << "Main descriptor set allocator creation skipped: no requirements have been added in the initialization phase of the engine." << bg2e_log_end;
        return;
    }
    bg2e_log_debug << "Initializing main descriptor set allocator with " << _initPoolRatios.size() << " pool size ratio requirements." << bg2e_log_end;
    
    if (_initialized)
    {
        throw std::runtime_error("DescriptorSetAllocator already initialized.");
    }
    
    _poolSizes.clear();
    for (auto ratio : _initPoolRatios)
    {
        VkDescriptorPoolSize size = {};
        size.type = ratio.type;
        size.descriptorCount = uint32_t(ratio.ratio * _initMaxSets);
        _poolSizes.push_back(size);
    }
    
    VkDescriptorPool newPool = createPool(_initMaxSets);
    _setsPerPool = uint32_t(std::round(float(_initMaxSets) * 1.5f));
    _readyPools.push_back(newPool);
    
    _initialized = true;
}

void DescriptorSetAllocator::initPool(
    uint32_t maxSets,
    std::vector<PoolSizeRatio> poolRatios
) {
    requirePoolSizeRatio(maxSets, poolRatios);
    initPool();
}

VkDescriptorPool DescriptorSetAllocator::getPool()
{
    VkDescriptorPool newPool;
    if (_readyPools.size() != 0)
    {
        newPool = _readyPools.back();
        _readyPools.pop_back();
    }
    else
    {
        newPool = createPool(_setsPerPool);
        
        _setsPerPool = uint32_t(std::round(float(_setsPerPool) * 1.5f));
        if (_setsPerPool > 4096) {
            _setsPerPool = 4096;
        }
    }
    return newPool;
}

VkDescriptorPool DescriptorSetAllocator::createPool(uint32_t setCount)
{
    VkDescriptorPoolCreateInfo  poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = setCount;
    poolInfo.poolSizeCount = uint32_t(_poolSizes.size());
    poolInfo.pPoolSizes = _poolSizes.data();

    VkDescriptorPool pool;
    vkCreateDescriptorPool(_vulkan->device().handle(), &poolInfo, nullptr, &pool);
    return pool;
}

void DescriptorSetAllocator::clearDescriptors()
{
    for (auto p : _readyPools)
    {
        vkResetDescriptorPool(_vulkan->device().handle(), p, 0);
    }
    for (auto p : _fullPools)
    {
        vkResetDescriptorPool(_vulkan->device().handle(), p, 0);
        _readyPools.push_back(p);
    }
    _fullPools.clear();
}

void DescriptorSetAllocator::destroy()
{
    for (auto p : _readyPools)
    {
        vkDestroyDescriptorPool(_vulkan->device().handle(), p, nullptr);
    }
    _readyPools.clear();
    for (auto p : _fullPools)
    {
        vkDestroyDescriptorPool(_vulkan->device().handle(), p, nullptr);
    }
    _fullPools.clear();
}

VkDescriptorSet DescriptorSetAllocator::allocateRaw(VkDescriptorSetLayout layout, void* pNext)
{
    VkDescriptorPool poolToUse = getPool();
    
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = pNext;
    allocInfo.descriptorPool = poolToUse;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;
    
    VkDescriptorSet descriptorSet;
    auto result = vkAllocateDescriptorSets(_vulkan->device().handle(), &allocInfo, &descriptorSet);
    
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
        _fullPools.push_back(poolToUse);
        
        poolToUse = getPool();
        allocInfo.descriptorPool = poolToUse;
        
        if (vkAllocateDescriptorSets(_vulkan->device().handle(), &allocInfo, &descriptorSet) != VK_SUCCESS)
        {
            throw new std::runtime_error("Could not allocate descriptor set. Review the configuration of the descriptor pool ratios in initPools() call. Make sure you have included all the descriptor types you are going to use in the descriptor set layout.");
        }
    }
    
    _readyPools.push_back(poolToUse);
    return descriptorSet;
}

DescriptorSet * DescriptorSetAllocator::allocate(VkDescriptorSetLayout layout, void* pNext)
{
    auto dsWrapper = new DescriptorSet();
    dsWrapper->init(_vulkan, allocateRaw(layout, pNext));
    return dsWrapper;
}

}
}
}

