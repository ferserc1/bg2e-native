
#include <bg2e/render/vulkan/geo/VertexDescription.hpp>

namespace bg2e::render::vulkan::geo {

template <class T>
VkVertexInputBindingDescription bindingDescription()
{
    VkVertexInputBindingDescription desc{};
    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexP>()
{
    VkVertexInputBindingDescription desc {};

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPN>()
{
    VkVertexInputBindingDescription desc {};

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPC>()
{
    VkVertexInputBindingDescription desc {};

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPU>()
{
    VkVertexInputBindingDescription desc {};

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNU>()
{
    VkVertexInputBindingDescription desc {};

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNC>()
{
    VkVertexInputBindingDescription desc {};

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNUC>()
{
    VkVertexInputBindingDescription desc {};

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNUT>()
{
    VkVertexInputBindingDescription desc {};

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNUUT>()
{
    VkVertexInputBindingDescription desc {};

    return desc;
}

template <class T>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> desc{};
    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexP>()
{
    std::vector<VkVertexInputAttributeDescription> desc;
    
    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPN>()
{
    std::vector<VkVertexInputAttributeDescription> desc;
    
    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPC>()
{
    std::vector<VkVertexInputAttributeDescription> desc;
    
    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPU>()
{
    std::vector<VkVertexInputAttributeDescription> desc;
    
    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNU>()
{
    std::vector<VkVertexInputAttributeDescription> desc;
    
    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNC>()
{
    std::vector<VkVertexInputAttributeDescription> desc;
    
    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNUC>()
{
    std::vector<VkVertexInputAttributeDescription> desc;
    
    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNUT>()
{
    std::vector<VkVertexInputAttributeDescription> desc;
    
    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNUUT>()
{
    std::vector<VkVertexInputAttributeDescription> desc;
    
    return desc;
}


}
