
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
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc {};

    desc.binding = 0;
    desc.stride = sizeof(VertexP);
	desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPN>()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexP);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPC>()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexP);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPU>()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexP);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNU>()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexP);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNC>()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexP);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNUC>()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexP);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNUT>()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexP);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}

template <>
VkVertexInputBindingDescription bindingDescription<bg2e::geo::VertexPNUUT>()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexP);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

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
    using namespace bg2e::geo;
    std::vector<VkVertexInputAttributeDescription> desc{};
    desc.resize(1);

    desc[0].binding = 0;
    desc[0].location = 0;
    desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[0].offset = offsetof(VertexP, position);

    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPN>()
{
    using namespace bg2e::geo;
    std::vector<VkVertexInputAttributeDescription> desc{};
    desc.resize(2);

    desc[0].binding = 0;
    desc[0].location = 0;
    desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[0].offset = offsetof(VertexPN, position);

	desc[1].binding = 0;
	desc[1].location = 1;
	desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	desc[1].offset = offsetof(VertexPN, normal);

    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPC>()
{
    using namespace bg2e::geo;
    std::vector<VkVertexInputAttributeDescription> desc{};
    desc.resize(2);

    desc[0].binding = 0;
    desc[0].location = 0;
    desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[0].offset = offsetof(VertexPC, position);

    desc[1].binding = 0;
    desc[1].location = 1;
    desc[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    desc[1].offset = offsetof(VertexPC, color);

    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPU>()
{
    using namespace bg2e::geo;
    std::vector<VkVertexInputAttributeDescription> desc{};
    desc.resize(2);

    desc[0].binding = 0;
    desc[0].location = 0;
    desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[0].offset = offsetof(VertexPU, position);

    desc[1].binding = 0;
    desc[1].location = 1;
    desc[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    desc[1].offset = offsetof(VertexPU, texCoord0);

    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNU>()
{
    using namespace bg2e::geo;
    std::vector<VkVertexInputAttributeDescription> desc{};
    desc.resize(3);

    desc[0].binding = 0;
    desc[0].location = 0;
    desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[0].offset = offsetof(VertexPNU, position);

    desc[1].binding = 0;
    desc[1].location = 1;
    desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[1].offset = offsetof(VertexPNU, normal);

    desc[2].binding = 0;
    desc[2].location = 2;
    desc[2].format = VK_FORMAT_R32G32_SFLOAT;
    desc[2].offset = offsetof(VertexPNU, texCoord0);

    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNC>()
{
    using namespace bg2e::geo;
    std::vector<VkVertexInputAttributeDescription> desc{};
    desc.resize(3);

    desc[0].binding = 0;
    desc[0].location = 0;
    desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[0].offset = offsetof(VertexPNC, position);

    desc[1].binding = 0;
    desc[1].location = 1;
    desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[1].offset = offsetof(VertexPNC, normal);

    desc[2].binding = 0;
    desc[2].location = 2;
    desc[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    desc[2].offset = offsetof(VertexPNC, color);

    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNUC>()
{
    using namespace bg2e::geo;
    std::vector<VkVertexInputAttributeDescription> desc{};
    desc.resize(4);

    desc[0].binding = 0;
    desc[0].location = 0;
    desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[0].offset = offsetof(VertexPNUC, position);

    desc[1].binding = 0;
    desc[1].location = 1;
    desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[1].offset = offsetof(VertexPNUC, normal);

    desc[2].binding = 0;
    desc[2].location = 2;
    desc[2].format = VK_FORMAT_R32G32_SFLOAT;
    desc[2].offset = offsetof(VertexPNUC, texCoord0);

    desc[3].binding = 0;
    desc[3].location = 3;
    desc[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    desc[3].offset = offsetof(VertexPNUC, color);

    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNUT>()
{
    using namespace bg2e::geo;
    std::vector<VkVertexInputAttributeDescription> desc{};
    desc.resize(4);

    desc[0].binding = 0;
    desc[0].location = 0;
    desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[0].offset = offsetof(VertexPNUT, position);

    desc[1].binding = 0;
    desc[1].location = 1;
    desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[1].offset = offsetof(VertexPNUT, normal);

    desc[2].binding = 0;
    desc[2].location = 2;
    desc[2].format = VK_FORMAT_R32G32_SFLOAT;
    desc[2].offset = offsetof(VertexPNUT, texCoord0);

    desc[3].binding = 0;
    desc[3].location = 3;
    desc[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[3].offset = offsetof(VertexPNUT, tangent);

    return desc;
}

template <>
std::vector<VkVertexInputAttributeDescription> attributeDescriptions<bg2e::geo::VertexPNUUT>()
{
    using namespace bg2e::geo;
    std::vector<VkVertexInputAttributeDescription> desc{};
    desc.resize(5);

    desc[0].binding = 0;
    desc[0].location = 0;
    desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[0].offset = offsetof(VertexPNUUT, position);

    desc[1].binding = 0;
    desc[1].location = 1;
    desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[1].offset = offsetof(VertexPNUUT, normal);

    desc[2].binding = 0;
    desc[2].location = 2;
    desc[2].format = VK_FORMAT_R32G32_SFLOAT;
    desc[2].offset = offsetof(VertexPNUUT, texCoord0);

    desc[3].binding = 0;
    desc[3].location = 3;
    desc[3].format = VK_FORMAT_R32G32_SFLOAT;
    desc[3].offset = offsetof(VertexPNUUT, texCoord0);

    desc[4].binding = 0;
    desc[4].location = 4;
    desc[4].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[4].offset = offsetof(VertexPNUUT, tangent);

    return desc;
}


}
