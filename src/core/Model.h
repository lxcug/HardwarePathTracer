//
// Created by HUSTLX on 2024/10/14.
//

#ifndef HARDWAREPATHTRACER_MODEL_H
#define HARDWAREPATHTRACER_MODEL_H

#include "core/Core.h"
#include "core/buffer/VertexBuffer.h"
#include "core/buffer/IndexBuffer.h"
#include "core/texture/Texture2D.h"
#include <filesystem>
#include <vector>


namespace HWPT {
    class Model {
    public:
        Model(const std::filesystem::path& ModelPath, const std::filesystem::path& TexturePath);

        ~Model();

        void LoadModel(const std::filesystem::path& ModelPath);

        void Bind(VkCommandBuffer CommandBuffer);

        auto GetTexture() -> Texture2D* {
            return m_texture;
        }

        [[nodiscard]] auto GetIndexCount() const -> uint {
            return m_indexBuffer->GetIndexCount();
        }

    private:
        Texture2D* m_texture = nullptr;
        IndexBuffer* m_indexBuffer = nullptr;
        VertexBuffer* m_vertexBuffer = nullptr;
    };

}  // namespace HWPT

#endif //HARDWAREPATHTRACER_MODEL_H
