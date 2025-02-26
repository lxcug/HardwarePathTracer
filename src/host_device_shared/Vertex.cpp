//
// Created by HUSTLX on 2025/1/15.
//

#include "Vertex.h"


BEGIN_SHADOWY_NAMESPACE

    auto Vertex::GetBindingDescription() -> VkVertexInputBindingDescription
    {
        VkVertexInputBindingDescription BindingDescription{};

        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(Vertex);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    auto Vertex::GetAttributeDescriptions() -> std::array<VkVertexInputAttributeDescription, 6>
    {
        std::array<VkVertexInputAttributeDescription, 6> AttributeDescriptions{};
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

        AttributeDescriptions[4].binding = 0;
        AttributeDescriptions[4].location = 4;
        AttributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[4].offset = offsetof(Vertex, Tangent);

        AttributeDescriptions[5].binding = 0;
        AttributeDescriptions[5].location = 3;
        AttributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[5].offset = offsetof(Vertex, Bitangent);

        return AttributeDescriptions;
    }

END_SHADOWY_NAME_SPACE
