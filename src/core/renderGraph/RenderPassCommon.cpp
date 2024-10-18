//
// Created by HUSTLX on 2024/10/18.
//

#include "RenderPassCommon.h"


namespace HWPT {
    auto GetVKPrimitiveType(PrimitiveType Type) -> VkPrimitiveTopology {
        switch (Type) {
            case PrimitiveType::Triangle:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case PrimitiveType::TriangleStrip:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case PrimitiveType::TriangleFan:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
            case PrimitiveType::Line:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case PrimitiveType::LineStrip:
                return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            case PrimitiveType::Point:
                return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            default:
                throw std::runtime_error("Unsupported PrimitiveType");
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }
}  // namespace HWPT
