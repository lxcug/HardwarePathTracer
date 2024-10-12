//
// Created by HUSTLX on 2024/10/7.
//

#ifndef HARDWAREPATHTRACER_VERTEXBUFFER_H
#define HARDWAREPATHTRACER_VERTEXBUFFER_H

#include "core/Core.h"
#include "vulkan/vulkan.h"
#include <glm/glm.hpp>
#include <array>


namespace HWPT {
    struct Vertex {
        glm::vec3 Pos;
        glm::vec3 Color;
        glm::vec2 TexCoord;

        Vertex(const glm::vec3& Pos, const glm::vec3 &Color, const glm::vec2& TexCoord):
        Pos(Pos), Color(Color), TexCoord(TexCoord) {}

        static auto GetBindingDescription() -> VkVertexInputBindingDescription;

        static auto GetAttributeDescriptions() -> std::array<VkVertexInputAttributeDescription, 3>;
    };

    class VertexBuffer {
    public:
        VertexBuffer(VkDeviceSize Size, const void* Data);

        VertexBuffer(VkDeviceSize Size, const Vertex* Data);

        ~VertexBuffer();

        void Bind(VkCommandBuffer CommandBuffer);

        auto GetHandle() -> VkBuffer& {
            return m_vertexBuffer;
        }

    private:
        VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_VERTEXBUFFER_H
