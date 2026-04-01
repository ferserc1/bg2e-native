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

