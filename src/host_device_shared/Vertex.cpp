//
// Created by HUSTLX on 2025/1/15.
//

#include "Vertex.h"


BEGIN_HWPT_NAMESPACE

    auto Vertex::GetBindingDescription() -> VkVertexInputBindingDescription
    {
        VkVertexInputBindingDescription BindingDescription{};

        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(Vertex);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    auto Vertex::GetAttributeDescriptions() -> std::array<VkVertexInputAttributeDescription, 4>
    {
        std::array<VkVertexInputAttributeDescription, 4> AttributeDescriptions{};
        AttributeDescriptions[0].binding = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[0].offset = offsetof(Vertex, Pos);

        AttributeDescriptions[1].binding = 0;
        AttributeDescriptions[1].location = 1;
        AttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[1].offset = offsetof(Vertex, Normal);

        AttributeDescriptions[2].binding = 0;
        AttributeDescriptions[2].location = 2;
        AttributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[2].offset = offsetof(Vertex, Color);

        AttributeDescriptions[3].binding = 0;
        AttributeDescriptions[3].location = 3;
        AttributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptions[3].offset = offsetof(Vertex, TexCoord);

        return AttributeDescriptions;
    }

END_HWPT_NAME_SPACE
