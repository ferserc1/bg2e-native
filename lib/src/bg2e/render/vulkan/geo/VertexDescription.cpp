
#include <bg2e/render/vulkan/geo/VertexDescription.hpp>

namespace bg2e::render::vulkan::geo {


VkVertexInputBindingDescription bindingDescriptionP()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc {};

    desc.binding = 0;
    desc.stride = sizeof(VertexP);
	desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}


VkVertexInputBindingDescription bindingDescriptionPN()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexPN);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}


VkVertexInputBindingDescription bindingDescriptionPC()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexPC);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}


VkVertexInputBindingDescription bindingDescriptionPU()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexPU);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}


VkVertexInputBindingDescription bindingDescriptionPNU()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexPNU);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}


VkVertexInputBindingDescription bindingDescriptionPNC()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexPNC);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}


VkVertexInputBindingDescription bindingDescriptionPNUC()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexPNUC);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}


VkVertexInputBindingDescription bindingDescriptionPNUT()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexPNUT);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}


VkVertexInputBindingDescription bindingDescriptionPNUUT()
{
    using namespace bg2e::geo;
    VkVertexInputBindingDescription desc{};

    desc.binding = 0;
    desc.stride = sizeof(VertexPNUUT);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return desc;
}

std::vector<VkVertexInputAttributeDescription> attributeDescriptionsP()
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


std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPN()
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


std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPC()
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


std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPU()
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
    desc[1].format = VK_FORMAT_R32G32_SFLOAT;
    desc[1].offset = offsetof(VertexPU, texCoord0);

    return desc;
}


std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPNU()
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


std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPNC()
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


std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPNUC()
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


std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPNUT()
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


std::vector<VkVertexInputAttributeDescription> attributeDescriptionsPNUUT()
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
    desc[3].offset = offsetof(VertexPNUUT, texCoord1);

    desc[4].binding = 0;
    desc[4].location = 4;
    desc[4].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[4].offset = offsetof(VertexPNUUT, tangent);

    return desc;
}


}
