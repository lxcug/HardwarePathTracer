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
#include "host_device_shared/Material.h"
#include "assimp/scene.h"
#include "assimp/Importer.hpp"


namespace Shadowy {

    class Mesh
    {
    public:
        Mesh(aiMesh* AIMesh, const aiScene* Scene, const glm::mat4& Transform);

        ~Mesh()
        {
            delete m_vertexBuffer;
            delete m_indexBuffer;
            delete m_materialIndexBuffer;
        }

        void Init(aiMesh* AIMesh, const aiScene* Scene, const glm::mat4& Transform);

        [[nodiscard]] auto GetBLASBuildInput() const -> BLASBuildInput;

        [[nodiscard]] auto
        GetTLASBuildInput(const ASBuilder *AccelBuilder) const -> VkAccelerationStructureInstanceKHR;

        uint m_instanceID = 0;
        // Rotate to Shadowy CoordSystem
        glm::mat4 m_transform = glm::identity<glm::mat4>();

        IndexBuffer *m_indexBuffer = nullptr;
        VertexBuffer *m_vertexBuffer = nullptr;
        ArbitraryBuffer *m_materialIndexBuffer = nullptr;  // Store Local Material Index for this Mesh

        ModelDesc m_meshDesc{};
        std::string m_name;
    };

    class Model
    {
    public:
        explicit Model(const std::filesystem::path& ModelPath);

        ~Model()
        {
            for (const auto Mesh : m_meshes)
            {
                delete Mesh;
            }
            delete m_materialBuffer;
        }

        void Init(const std::filesystem::path& ModelPath);

        void ProcessNode(aiNode* Node, const aiScene* Scene, const glm::mat4& ParentTransform);

        auto ProcessMeshMaterial(aiMaterial* AIMaterial, const aiScene* Scene) -> Material;

        [[nodiscard]] auto
        GetTLASBuildInput(const ASBuilder *AccelBuilder) const -> VkAccelerationStructureInstanceKHR;


        // TODO
        glm::mat4 m_transform = glm::identity<glm::mat4>();
        std::filesystem::path m_path;
        std::vector<Mesh*> m_meshes;
        std::vector<Material> m_materials;
        ArbitraryBuffer *m_materialBuffer = nullptr;  // Store All Materials for Meshes

        Assimp::Importer Importer;
        const aiScene* m_aiScene = nullptr;

    private:
        static auto AIMatrix4x4ToGlm(const aiMatrix4x4& Mat) -> glm::mat4;
    };

    // class ObjModel {
    // public:
    //     explicit ObjModel(const std::filesystem::path &ModelPath);
    //
    //     ~ObjModel();
    //
    //     void LoadModel(const std::filesystem::path &ModelPath);
    //
    //     void Bind(VkCommandBuffer CommandBuffer);
    //
    //     [[nodiscard]] auto GetVertexCount() const -> uint {
    //         return m_vertexBuffer->GetVertexCount();
    //     }
    //
    //     [[nodiscard]] auto GetIndexCount() const -> uint {
    //         return m_indexBuffer->GetIndexCount();
    //     }
    //
    //     auto GetVertexBuffer() -> VertexBuffer * {
    //         return m_vertexBuffer;
    //     }
    //
    //     auto GetIndexBuffer() -> IndexBuffer * {
    //         return m_indexBuffer;
    //     }
    //
    //     auto GetVertexBufferLayout() -> VertexBufferLayout * {
    //         return m_vertexBuffer->GetLayout();
    //     }
    //
    //     void DrawIndexed(VkCommandBuffer CommandBuffer);
    //
    //     [[nodiscard]] auto GetBLASBuildInput() const -> BLASBuildInput;
    //
    //     [[nodiscard]] auto
    //     GetTLASBuildInput(const ASBuilder *AccelBuilder) const -> VkAccelerationStructureInstanceKHR;
    //
    //     void SetModelTransform(const glm::mat4 &ModelTransform) {
    //         m_transform = ModelTransform;
    //     }
    //
    //     auto GetModelTransform() -> const glm::mat4 & {
    //         return m_transform;
    //     }
    //
    //     void SetInstanceID(uint InstanceID) {
    //         m_instanceID = InstanceID;
    //     }
    //
    //     [[nodiscard]] auto GetInstanceID() const -> uint {
    //         return m_instanceID;
    //     }
    //
    //     void SetModelName(const std::string &Name) {
    //         m_modelName = Name;
    //     }
    //
    //     [[nodiscard]] auto GetModelName() -> const std::string & {
    //         return m_modelName;
    //     }
    //
    //     [[nodiscard]] auto GetModelDesc() const -> const ModelDesc & {
    //         return m_modelDesc;
    //     }
    //
    //     void SetModelDescTextureOffset(int Offset) {
    //         m_modelDesc.TextureIndexOffset = Offset;
    //     }
    //
    //     [[nodiscard]] auto GetModelTexturePaths() const -> const std::vector<std::string> & {
    //         return m_textureNames;
    //     }
    //
    // private:
    //     std::string m_modelName = "Empty";
    //     glm::mat4 m_transform = glm::identity<glm::mat4>();
    //     uint m_instanceID = 0;
    //
    //     std::vector<std::string> m_textureNames;
    //     std::vector<Material> m_materials;
    //     std::vector<int> m_materialIndex;
    //
    //     IndexBuffer *m_indexBuffer = nullptr;
    //     VertexBuffer *m_vertexBuffer = nullptr;
    //     ArbitraryBuffer *m_materialBuffer = nullptr;
    //     ArbitraryBuffer *m_materialIndexBuffer = nullptr;
    //
    //     ModelDesc m_modelDesc;
    // };

}  // namespace Shadowy

#endif //HARDWAREPATHTRACER_MODEL_H
