//
// Created by HUSTLX on 2024/10/17.
//

#ifndef HARDWAREPATHTRACER_VERTEXBUFFERLAYOUT_H
#define HARDWAREPATHTRACER_VERTEXBUFFERLAYOUT_H

#include "core/Core.h"
#include <vector>


namespace HWPT {
    enum class VertexAttributeDataType : uint8_t {
        None = 0,
        Float, Float2, Float3, Float4,
        UInt, UInt2, UInt3, UInt4,
        Int, Int2, Int3, Int4,
        Mat3, Mat4,
        Bool
    };

    static auto GetVertexAttributeDataTypeSize(VertexAttributeDataType DataType) -> uint;

    static auto GetVertexAttributeVKFormat(VertexAttributeDataType DataType) -> VkFormat;

    struct VertexAttribute {
        VertexAttribute( VertexAttributeDataType Type, std::string InName)
                : Name(std::move(InName)), DataType(Type) {}

        std::string Name;
        VertexAttributeDataType DataType;
        uint Offset = 0;
    };

    struct VertexBufferLayout {
        VertexBufferLayout(const std::initializer_list<VertexAttribute> &InitializerList);

        [[nodiscard]] auto GetBindingDescription() const -> VkVertexInputBindingDescription;

        [[nodiscard]] auto GetAttributeDescriptions() const -> std::vector<VkVertexInputAttributeDescription>;

        std::vector<VertexAttribute> Attributes;
        uint Stride = 0;
    };

}  // namespace HWPT

#endif //HARDWAREPATHTRACER_VERTEXBUFFERLAYOUT_H
