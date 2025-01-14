//
// Created by HUSTLX on 2024/10/7.
//

#ifndef HARDWAREPATHTRACER_VERTEXBUFFER_H
#define HARDWAREPATHTRACER_VERTEXBUFFER_H

#include "core/Core.h"
#include "vulkan/vulkan.h"
#include <glm/glm.hpp>
#include <array>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/hash.hpp>
#include <unordered_map>
#include <utility>
#include "VertexBufferLayout.h"


namespace HWPT {
    struct Vertex {
        glm::vec3 Pos;
        uint PosPadding;
        glm::vec3 Normal;
        uint NormalPadding;
        glm::vec3 Color;
        uint ColorPadding;
        glm::vec2 TexCoord;

        Vertex() = default;

        auto operator==(const Vertex &Other) const -> bool {
            return Pos == Other.Pos && Normal == Other.Normal;
        }

        static auto GetBindingDescription() -> VkVertexInputBindingDescription;

        static auto GetAttributeDescriptions() -> std::array<VkVertexInputAttributeDescription, 4>;
    };

    class VertexBuffer {
    public:
        VertexBuffer(VkDeviceSize Size, const void *Data);

        VertexBuffer(VkDeviceSize Size, const Vertex *Data);

        void SetLayout(const std::initializer_list<VertexAttribute> &Attributes);

        auto GetLayout() -> VertexBufferLayout * {
            return m_layout;
        }

        ~VertexBuffer();

        void Bind(VkCommandBuffer CommandBuffer);

        auto GetHandle() -> VkBuffer & {
            return m_vertexBuffer;
        }

        [[nodiscard]] auto GetVertexCount() const -> uint {
            // If vertex type is not 'Vertex', we don't cal the vertex count explicitly in vertex buffer
            Check(m_vertexCount != 0);
            return m_vertexCount;
        }

    private:
        VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
        VertexBufferLayout *m_layout = nullptr;
        uint m_vertexCount = 0;
    };
}  // namespace HWPT

namespace std {
    template<>
    struct hash<HWPT::Vertex> {
        auto operator()(HWPT::Vertex const &_Vertex) const -> size_t {
            return ((hash<glm::vec3>()(_Vertex.Pos) ^
                     (hash<glm::vec3>()(_Vertex.Normal) << 1)) >> 1) ^
                   (hash<glm::vec2>()(_Vertex.TexCoord) << 1);
        }
    };
}  // namespace std

#endif //HARDWAREPATHTRACER_VERTEXBUFFER_H
