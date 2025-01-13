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
#include "core/acceleration_structure/AccelerationStructure.h"
#include "core/material/Material.h"


namespace HWPT {

    struct ModelDesc {
        VkDeviceAddress VertexBufferAddress;
        VkDeviceAddress IndexBufferAddress;
        VkDeviceAddress MaterialBufferAddress;  // Material Array of All Triangles' Address
        VkDeviceAddress MaterialIndexBufferAddress;  // Material Index of a Triangle
        int TextureIndexOffset = -1;
    };

    class Model {
    public:
        Model(const std::filesystem::path &ModelPath, const std::filesystem::path &TexturePath,
              bool GenerateMips = false);

        ~Model();

        void LoadModel(const std::filesystem::path &ModelPath);

        void Bind(VkCommandBuffer CommandBuffer);

        auto GetTexture() -> Texture2D * {
            return m_texture;
        }

        [[nodiscard]] auto GetVertexCount() const -> uint {
            return m_vertexBuffer->GetVertexCount();
        }

        [[nodiscard]] auto GetIndexCount() const -> uint {
            return m_indexBuffer->GetIndexCount();
        }

        auto GetVertexBuffer() -> VertexBuffer * {
            return m_vertexBuffer;
        }

        auto GetIndexBuffer() -> IndexBuffer * {
            return m_indexBuffer;
        }

        auto GetVertexBufferLayout() -> VertexBufferLayout * {
            return m_vertexBuffer->GetLayout();
        }

        void DrawIndexed(VkCommandBuffer CommandBuffer);

        [[nodiscard]] auto GetBLASBuildInput() const -> BLASBuildInput;

        [[nodiscard]] auto
        GetTLASBuildInput(ASBuilder *AccelBuilder) const -> VkAccelerationStructureInstanceKHR;

        void SetModelTransform(const glm::mat4 &ModelTransform) {
            m_transform = ModelTransform;
        }

        auto GetModelTransform() -> const glm::mat4 & {
            return m_transform;
        }

        void SetInstanceID(uint InstanceID) {
            m_instanceID = InstanceID;
        }

        [[nodiscard]] auto GetInstanceID() const -> uint {
            return m_instanceID;
        }

        void SetModelName(const std::string &Name) {
            m_modelName = Name;
        }

        [[nodiscard]] auto GetModelName() -> const std::string & {
            return m_modelName;
        }

        [[nodiscard]] auto GetModelDesc() const -> const ModelDesc & {
            return m_modelDesc;
        }

        void SetModelDescTextureOffset(int Offset) {
            m_modelDesc.TextureIndexOffset = Offset;
        }

        [[nodiscard]] auto GetModelTexturePaths() const -> const std::vector<std::string> & {
            return m_textureNames;
        }

    private:
        std::string m_modelName = "Empty";
        bool m_generateMips = false;
        glm::mat4 m_transform = glm::identity<glm::mat4>();
        uint m_instanceID = 0;

        // Legacy for Graphics Pipeline, to refactor
        Texture2D *m_texture = nullptr;

        std::vector<std::string> m_textureNames;
        std::vector<Material> m_materials;
        std::vector<int> m_materialIndex;

        IndexBuffer *m_indexBuffer = nullptr;
        VertexBuffer *m_vertexBuffer = nullptr;
        ArbitraryBuffer *m_materialBuffer = nullptr;
        ArbitraryBuffer *m_materialIndexBuffer = nullptr;

        ModelDesc m_modelDesc;
    };

}  // namespace HWPT

#endif //HARDWAREPATHTRACER_MODEL_H
