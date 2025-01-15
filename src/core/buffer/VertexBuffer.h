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

#include "VertexBufferLayout.h"
#include "host_device_shared/Vertex.h"


namespace HWPT {

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

#endif //HARDWAREPATHTRACER_VERTEXBUFFER_H
