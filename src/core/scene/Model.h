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
#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "host_device_shared/ShadowyMaterial.h"


namespace Shadowy {

    class Mesh
    {
    public:
        Mesh(aiMesh* AIMesh, const aiScene* Scene, const glm::mat4& Transform);

        ~Mesh()
        {
            delete m_vertexBuffer;
            delete m_indexBuffer;
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

        InstanceData m_meshDesc{};
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

        auto ProcessMaterial(aiMaterial* AIMaterial) -> InputMaterial;

        void UpdateMaterialBuffer();

        [[nodiscard]] auto
        GetTLASBuildInput(const ASBuilder *AccelBuilder) const -> VkAccelerationStructureInstanceKHR;

        auto ProcessLight() -> std::vector<Light>;


        // TODO
        std::string m_name;
        glm::mat4 m_transform = glm::identity<glm::mat4>();
        std::filesystem::path m_path;
        std::vector<Mesh*> m_meshes;
        std::vector<InputMaterial> m_materials;
        ArbitraryBuffer *m_materialBuffer = nullptr;  // Store All Materials for Meshes

        Assimp::Importer Importer;
        const aiScene* m_aiScene = nullptr;

    private:
        static auto GetAbsoluteNodeTransform(const aiNode* Node) -> glm::mat4;

        static auto AIMatrix4x4ToGlm(const aiMatrix4x4& Mat) -> glm::mat4;

        static auto AIVec3ToGlm(const aiVector3f& Vec) -> glm::vec3;

        static auto AIColorToGlm(const aiColor3D& Color) -> glm::vec3;

        static auto AILightTypeToShadowy(aiLightSourceType Type) -> LightType;

        static void GetAIMaterialColor(aiMaterial* AIMaterial,
                                       const char* pKey, unsigned int Type, unsigned int Idx,
                                       glm::vec3& Value);

        static void GetAIMaterialFloat(aiMaterial* AIMaterial,
                                       const char* pKey, unsigned int Type, unsigned int Idx,
                                       float& Value);

        static void GetAIMaterialInt(aiMaterial* AIMaterial,
                                     const char* pKey, unsigned int Type, unsigned int Idx,
                                     int& Value);

        static auto GetAIMaterialString(aiMaterial* AIMaterial,
                                        const char* pKey, unsigned int Type, unsigned int Idx)
                                        -> std::tuple<bool, std::string>;

        static auto GetAIMaterialTexturePath(aiMaterial* AIMaterial, aiTextureType Type,
                                             uint Index) -> std::tuple<bool, std::string>;
    };

}  // namespace Shadowy

#endif //HARDWAREPATHTRACER_MODEL_H
