//
// Created by HUSTLX on 2024/10/17.
//

#include "VertexBufferLayout.h"


namespace HWPT {
    static auto GetVertexAttributeDataTypeSize(VertexAttributeDataType DataType) -> uint {
        switch (DataType) {
            case VertexAttributeDataType::Float: [[fallthrough]];
            case VertexAttributeDataType::Int: [[fallthrough]];
            case VertexAttributeDataType::UInt:
                return 4;
            case VertexAttributeDataType::Float2: [[fallthrough]];
            case VertexAttributeDataType::Int2: [[fallthrough]];
            case VertexAttributeDataType::UInt2:
                return 8;
            case VertexAttributeDataType::Float3: [[fallthrough]];
            case VertexAttributeDataType::Int3: [[fallthrough]];
            case VertexAttributeDataType::UInt3:
                return 12;
            case VertexAttributeDataType::Float4: [[fallthrough]];
            case VertexAttributeDataType::Int4: [[fallthrough]];
            case VertexAttributeDataType::UInt4:
                return 16;
            case VertexAttributeDataType::Mat3:
                return 36;
            case VertexAttributeDataType::Mat4:
                return 64;
            case VertexAttributeDataType::Bool:
                return 1;
            case VertexAttributeDataType::None: [[fallthrough]];
            default:
                throw std::runtime_error("Unsupported VertexAttribute DataType");
        }
    }

    static auto GetVertexAttributeVKFormat(VertexAttributeDataType DataType) -> VkFormat {
        switch (DataType) {
            case VertexAttributeDataType::Int:
                return VK_FORMAT_R32_SINT;
            case VertexAttributeDataType::Int2:
                return VK_FORMAT_R32G32_SINT;
            case VertexAttributeDataType::Int3:
                return VK_FORMAT_R32G32B32A32_SINT;
            case VertexAttributeDataType::Int4:
                return VK_FORMAT_R32G32B32A32_SINT;
            case VertexAttributeDataType::UInt:
                return VK_FORMAT_R32_UINT;
            case VertexAttributeDataType::UInt2:
                return VK_FORMAT_R32G32_UINT;
            case VertexAttributeDataType::UInt3:
                return VK_FORMAT_R32G32B32_UINT;
            case VertexAttributeDataType::UInt4:
                return VK_FORMAT_R32G32B32A32_UINT;
            case VertexAttributeDataType::Float:
                return VK_FORMAT_R32_SFLOAT;
            case VertexAttributeDataType::Float2:
                return VK_FORMAT_R32G32_SFLOAT;
            case VertexAttributeDataType::Float3:
                return VK_FORMAT_R32G32B32_SFLOAT;
            case VertexAttributeDataType::Float4:
                return VK_FORMAT_R32G32B32A32_SFLOAT;
            case VertexAttributeDataType::None: [[fallthrough]];
            default:
                throw std::runtime_error("Unsupported VertexAttribute DataType");
        }
    }

    VertexBufferLayout::VertexBufferLayout(const std::initializer_list<VertexAttribute> &InitializerList) {
        Attributes.assign(InitializerList);
        for (auto& Attrib : Attributes) {
            Attrib.Offset = Stride;
            Stride += GetVertexAttributeDataTypeSize(Attrib.DataType);
        }
    }

    auto VertexBufferLayout::GetBindingDescription() const -> VkVertexInputBindingDescription {
        VkVertexInputBindingDescription BindingDescription{};
        BindingDescription.binding = 0;
        BindingDescription.stride = Stride;
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    auto
    VertexBufferLayout::GetAttributeDescriptions() const -> std::vector<VkVertexInputAttributeDescription> {
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptions(Attributes.size());

        for (int i = 0; i < Attributes.size(); i++) {
            AttributeDescriptions[i].binding = 0;
            AttributeDescriptions[i].location = i;
            AttributeDescriptions[i].format = GetVertexAttributeVKFormat(Attributes[i].DataType);
            AttributeDescriptions[i].offset = Attributes[i].Offset;
        }

        return AttributeDescriptions;
    }
}  // namespace HWPT
