//
// Created by HUSTLX on 2025/1/15.
//

#ifndef VERTEX_H
#define VERTEX_H

#include "BaseDefinitions.h"

#ifdef __cplusplus
#include <vulkan/vulkan.h>
#include <array>
#endif


BEGIN_SHADOWY_NAMESPACE

struct Vertex
{
    float3 Pos;
    uint PosPadding;
    float3 Normal;
    uint NormalPadding;
    float3 Color;
    uint ColorPadding;
    float2 TexCoord;
    float2 TexCoordPadding;
    float3 Tangent;
    uint TangentPadding;
    float3 Bitangent;
    uint BitangentPadding;


#if IS_COMPILING_CPP
    Vertex() = default;

    auto operator==(const Vertex &Other) const -> bool {
        return Pos == Other.Pos && Normal == Other.Normal;
    }

    static auto GetBindingDescription() -> VkVertexInputBindingDescription;

    static auto GetAttributeDescriptions() -> std::array<VkVertexInputAttributeDescription, 6>;
#endif
};

struct Vertex3 {
    Vertex V0, V1, V2;
};

END_SHADOWY_NAME_SPACE

#if IS_COMPILING_CPP
#include <glm/gtx/hash.hpp>
template<>
struct std::hash<Shadowy::Vertex> {
    auto operator()(Shadowy::Vertex const &_Vertex) const noexcept -> size_t {
        return ((hash<glm::vec3>()(_Vertex.Pos) ^
                (hash<glm::vec3>()(_Vertex.Normal) << 1)) >> 1) ^
            (hash<glm::vec2>()(_Vertex.TexCoord) << 1);
    }
};  // namespace std
#endif

#endif //VERTEX_H
