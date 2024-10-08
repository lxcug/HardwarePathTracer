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

        Vertex(const glm::vec3& pos, const glm::vec3 &color): Pos(pos), Color(color) {}

        static VkVertexInputBindingDescription GetBindingDescription();

        static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions();
    };

    class VertexBuffer {
    public:
        VertexBuffer(VkDeviceSize Size, void* Data);

        ~VertexBuffer();

        VkBuffer& GetHandle() {
            return m_vertexBuffer;
        }

    private:
        VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
    };
}

#endif //HARDWAREPATHTRACER_VERTEXBUFFER_H
