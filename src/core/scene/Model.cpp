//
// Created by HUSTLX on 2024/10/14.
//

#define TINYOBJLOADER_IMPLEMENTATION

#include <tiny_obj_loader.h>
#include "core/scene/Model.h"
#include <unordered_map>
#include "assimp/postprocess.h"
#include "core/application/VulkanBackendApp.h"
#include "core/RHI.h"
#include "core/Utils.h"
#include "Scene.h"
#include "assimp/pbrmaterial.h"


namespace Shadowy {
    Mesh::Mesh(aiMesh* AIMesh, const aiScene* Scene, const glm::mat4& Transform)
    {
        Init(AIMesh, Scene, Transform);
    }

    void Mesh::Init(aiMesh* AIMesh, const aiScene* Scene, const glm::mat4& Transform)
    {
        m_instanceID = Scene::s_instanceIDCounter++;
        std::vector<Vertex> Vertices(AIMesh->mNumVertices);
        std::vector<uint> Indices(AIMesh->mNumFaces * 3);
        m_meshDesc.MaterialIndex = AIMesh->mMaterialIndex;

        m_transform = Transform;

        m_name = AIMesh->mName.data;

        // Process Vertices
        for (uint i = 0; i < AIMesh->mNumVertices; i++)
        {
            Vertex Vertex_{};
            Check(AIMesh->HasPositions());

            Vertex_.Pos = {
                AIMesh->mVertices[i].x, AIMesh->mVertices[i].y, AIMesh->mVertices[i].z
            };
            Vertex_.Normal = {
                AIMesh->mNormals[i].x, AIMesh->mNormals[i].y, AIMesh->mNormals[i].z
            };
            if (AIMesh->HasTangentsAndBitangents())
            {
                glm::vec3 Tangent = {AIMesh->mTangents[i].x, AIMesh->mTangents[i].y, AIMesh->mTangents[i].z};
                glm::vec3 Bitangent = {AIMesh->mBitangents[i].x, AIMesh->mBitangents[i].y, AIMesh->mBitangents[i].z};
                glm::vec3 CalBitangent = glm::normalize(glm::cross(Vertex_.Normal, Tangent));
                float Dot = glm::dot(CalBitangent, Bitangent);
                float Handedness = Dot >= 0.f ? 1.f : -1.f;
                Vertex_.Tangent = {Tangent, Handedness};
            }
            if (AIMesh->HasTextureCoords(0))
            {
                Vertex_.TexCoord = {
                    AIMesh->mTextureCoords[0][i].x, AIMesh->mTextureCoords[0][i].y
                };
            }
            if (AIMesh->HasVertexColors(0))
            {
                Vertex_.Color = {
                    AIMesh->mColors[0][i].r, AIMesh->mColors[0][i].g, AIMesh->mColors[0][i].b
                };
            }

            Vertices[i] = Vertex_;
        }

        // Process Indices
        for (uint i = 0; i < AIMesh->mNumFaces; i++)
        {
            const aiFace& Face = AIMesh->mFaces[i];
            for (uint j = 0; j < Face.mNumIndices; j++)
            {
                Indices[i * 3 + j] = Face.mIndices[j];
            }
        }

        m_vertexBuffer = new VertexBuffer(
            sizeof(Vertex) * Vertices.size(),
            Vertices.data()
        );
        m_vertexBuffer->SetLayout({
            {VertexAttributeDataType::Float3, "Pos"},
            {VertexAttributeDataType::Float3, "Normal"},
            {VertexAttributeDataType::Float3, "Color"},
            {VertexAttributeDataType::Float2, "TexCoord"}
        });
        m_indexBuffer = new IndexBuffer(
            Indices.size(),
            Indices.data()
        );

        m_meshDesc.VertexBufferAddress = RHI::GetBufferDeviceAddress(m_vertexBuffer->GetHandle());
        m_meshDesc.IndexBufferAddress = RHI::GetBufferDeviceAddress(m_indexBuffer->GetHandle());
    }

    auto Mesh::GetBLASBuildInput() const -> BLASBuildInput
    {
        VkDeviceAddress VertexBufferAddress = RHI::GetBufferDeviceAddress(
                m_vertexBuffer->GetHandle());
        VkDeviceAddress IndexBufferAddress = RHI::GetBufferDeviceAddress(
                m_indexBuffer->GetHandle());

        VkAccelerationStructureGeometryTrianglesDataKHR Triangles{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR
        };
        Triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        Triangles.vertexData.deviceAddress = VertexBufferAddress;
        Triangles.vertexStride = sizeof(Vertex);
        Triangles.maxVertex = m_vertexBuffer->GetVertexCount() - 1;
        Triangles.indexType = VK_INDEX_TYPE_UINT32;
        Triangles.indexData.deviceAddress = IndexBufferAddress;
        Triangles.transformData = {};

        // Create AS Geometry
        VkAccelerationStructureGeometryKHR ASGeometry{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR
        };
        ASGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        ASGeometry.geometry.triangles = Triangles;
//        ASGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;  // NOTE: Prohibit for invoke any hit shader

        // Create AS Build Range
        VkAccelerationStructureBuildRangeInfoKHR BuildRange;
        BuildRange.primitiveCount = m_indexBuffer->GetIndexCount() / 3;
        BuildRange.primitiveOffset = 0;
        BuildRange.firstVertex = 0;
        BuildRange.transformOffset = 0;

        BLASBuildInput Input;
        Input.ASGeometries.emplace_back(ASGeometry);
        Input.ASBuildRangeInfos.emplace_back(BuildRange);

        return Input;
    }

    auto Mesh::GetTLASBuildInput(
        const ASBuilder* AccelBuilder) const -> VkAccelerationStructureInstanceKHR
    {
        VkAccelerationStructureInstanceKHR Instance{};
        Instance.transform = Utils::GLMToVulkanMatrix(m_transform);
        Instance.instanceCustomIndex = m_instanceID;
        Instance.accelerationStructureReference = AccelBuilder->GetBLASDeviceAddress(m_instanceID);
        Instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        Instance.mask = 0xff; //  Only be hit if rayMask & instance.mask != 0
        Instance.instanceShaderBindingTableRecordOffset = 0;

        return Instance;
    }

    Model::Model(const std::filesystem::path& ModelPath) : m_path(ModelPath.parent_path())
    {
        Init(ModelPath);
    }

    void Model::Init(const std::filesystem::path& ModelPath)
    {
        m_aiScene = Importer.ReadFile(
            ModelPath.string(),
            aiProcess_Triangulate  | aiProcess_GenNormals | aiProcess_FlipUVs
            | aiProcess_GenUVCoords | aiProcess_CalcTangentSpace
        );

        if (!m_aiScene || m_aiScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_aiScene->mRootNode)
        {
            Check(false);
            return;
        }

        m_materials.resize(m_aiScene->mNumMaterials);

        auto IdentityTransform = glm::identity<glm::mat4>();
        ProcessNode(m_aiScene->mRootNode, m_aiScene, IdentityTransform);

        m_materialBuffer = new ArbitraryBuffer(
            sizeof(InputMaterial) * m_materials.size(),
            m_materials.data(),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        );

        auto MaterialBufferAddress = RHI::GetBufferDeviceAddress(m_materialBuffer->GetHandle());
        for (auto& Mesh : m_meshes)
        {
            Mesh->m_meshDesc.MaterialBufferAddress = MaterialBufferAddress;
        }
    }

    void Model::ProcessNode(aiNode* Node, const aiScene* Scene, const glm::mat4& ParentTransform)
    {
        glm::mat4 CurrentNodeTransform = ParentTransform * AIMatrix4x4ToGlm(Node->mTransformation);
        for (uint i = 0; i < Node->mNumMeshes; i++)
        {
            aiMesh* AIMesh = Scene->mMeshes[Node->mMeshes[i]];
            // Row Major to Col Major
            Mesh* ShadowyMesh = new Mesh(AIMesh, Scene, CurrentNodeTransform);
            m_meshes.push_back(ShadowyMesh);

            /* NOTE: Each AIMesh use only one material, otherwise the mesh will be splitted,
             *  use the original MaterialIndex loaded from file, since the materials are already deduplicated
             */
            const uint MaterialIndex = AIMesh->mMaterialIndex;
            m_materials[MaterialIndex] = ProcessMaterial(Scene->mMaterials[MaterialIndex]);
            ProcessMaterial(Scene->mMaterials[MaterialIndex]);
        }

        for (uint i = 0; i < Node->mNumChildren; i++)
        {
            ProcessNode(Node->mChildren[i], Scene, CurrentNodeTransform);
        }
    }

    auto Model::ProcessLight() -> std::vector<Light>
    {
        Check(m_aiScene);
        if (m_aiScene->mNumLights == 0)
        {
            std::cout << "No light exists in the scene";
        }

        glm::mat4 ParentNodeTransform = AIMatrix4x4ToGlm(m_aiScene->mRootNode->mTransformation);
        std::vector<Light> SceneLights(m_aiScene->mNumLights, Light());
        for (int i = 0; i < m_aiScene->mNumLights; i++)
        {
            aiLight* AILight = m_aiScene->mLights[i];
            Check(AILight->mType != aiLightSource_UNDEFINED);
            Light& ShadowyLight = SceneLights[i];
            ShadowyLight.Type = AILightTypeToShadowy(AILight->mType);
            glm::vec3 AILightColor = AIColorToGlm(AILight->mColorDiffuse);
            glm::mat4 Transform = GetAbsoluteNodeTransform(m_aiScene->mRootNode->FindNode(AILight->mName));
            glm::vec4 LocalPos = {AIVec3ToGlm(AILight->mPosition), 1.f};
            glm::vec4 WorldPos = Transform * LocalPos;
            ShadowyLight.Position = {WorldPos.x, WorldPos.y, WorldPos.z};
            glm::vec4 WorldDir = Transform * glm::vec4(AIVec3ToGlm(AILight->mDirection), 0.f);
            ShadowyLight.Direction = {WorldDir.x, WorldDir.y, WorldDir.z};

            const float Intensity = std::max(AILightColor.x, glm::max(AILightColor.y, AILightColor.z));
            ShadowyLight.Color = AILightColor / Intensity;
            ShadowyLight.Intensity = Intensity;
            if (ShadowyLight.Type == LightType::Point)
            {
                const float Constant = AILight->mAttenuationConstant;
                const float Linear = AILight->mAttenuationLinear;
                const float Quadratic = AILight->mAttenuationQuadratic;
                constexpr float AttenuationThreshold = 1e-2f;
                const float Range = (-Linear + std::sqrt(Linear * Linear - 4 * (Constant - 1.f / AttenuationThreshold) * Quadratic)) / (2 * Quadratic);
                ShadowyLight.HalfSinAngleOrRange = Range;
                ShadowyLight.Intensity = Intensity / (Range * Range);
            }
            else if (ShadowyLight.Type ==LightType::Directional)
            {
                ShadowyLight.Intensity /= 1e1f;  // TODO
            }
            ShadowyLight.InnerAngle = AILight->mAngleInnerCone;
            ShadowyLight.OuterAngle = AILight->mAngleOuterCone;
            ShadowyLight.Radius = .1f;
        }

        return SceneLights;
    }

    auto Model::GetAbsoluteNodeTransform(const aiNode* Node) -> glm::mat4
    {
        if (Node->mParent == nullptr)
        {
            return AIMatrix4x4ToGlm(Node->mTransformation);
        }
        return GetAbsoluteNodeTransform(Node->mParent) * AIMatrix4x4ToGlm(Node->mTransformation);
    }

    auto Model::AIMatrix4x4ToGlm(const aiMatrix4x4& Mat) -> glm::mat4
    {
        // Row Major to Col Major
        const glm::mat4 Ret = {
            Mat.a1, Mat.b1, Mat.c1, Mat.d1,
            Mat.a2, Mat.b2, Mat.c2, Mat.d2,
            Mat.a3, Mat.b3, Mat.c3, Mat.d3,
            Mat.a4, Mat.b4, Mat.c4, Mat.d4
        };
        return Ret;
    }

    auto Model::AIVec3ToGlm(const aiVector3f& Vec) -> glm::vec3
    {
        return {
            Vec.x, Vec.y, Vec.z
        };
    }

    auto Model::AIColorToGlm(const aiColor3D& Color) -> glm::vec3
    {
        return {
            Color.r, Color.g, Color.b
        };
    }

    auto Model::AILightTypeToShadowy(aiLightSourceType Type) -> LightType
    {
        switch (Type)
        {
        case aiLightSource_DIRECTIONAL:
            return LightType::Directional;
        case aiLightSource_POINT:
            return LightType::Point;
        case aiLightSource_SPOT:
            return LightType::Spot;
        case aiLightSource_AREA:
            return LightType::Rect;
        default:
            Check(false);
            return LightType::LightTypeMax;
        }
    }

    auto Model::ProcessMaterial(aiMaterial *AIMaterial) -> InputMaterial {
        InputMaterial Material{};

//        for (unsigned int i = 0; i < AIMaterial->mNumProperties; ++i) {
//            const aiMaterialProperty* property = AIMaterial->mProperties[i];
//            std::cout << "Property Key: " << property->mKey.C_Str() << std::endl;
//            std::cout << "Data Type: " << property->mType << std::endl;
//            std::cout << "Data Length: " << property->mDataLength << std::endl;
//            std::cout << "---------------------" << std::endl;
//        }

//        GetAIMaterialColor(AIMaterial, AI_MATKEY_COLOR_DIFFUSE, Material.albedo);
        GetAIMaterialColor(AIMaterial, AI_MATKEY_BASE_COLOR, Material.albedo);
        GetAIMaterialColor(AIMaterial, AI_MATKEY_COLOR_EMISSIVE, Material.emissive);
        GetAIMaterialColor(AIMaterial, AI_MATKEY_COLOR_TRANSPARENT, Material.transmission);
        GetAIMaterialColor(AIMaterial, AI_MATKEY_COLOR_SPECULAR, Material.specular);
        GetAIMaterialFloat(AIMaterial, AI_MATKEY_ROUGHNESS_FACTOR, Material.roughness);
        GetAIMaterialFloat(AIMaterial, AI_MATKEY_METALLIC_FACTOR, Material.metallic);
        GetAIMaterialFloat(AIMaterial, AI_MATKEY_OPACITY, Material.opacity);
        GetAIMaterialFloat(AIMaterial, AI_MATKEY_REFRACTI, Material.ior);
        GetAIMaterialInt(AIMaterial, AI_MATKEY_TWOSIDED, Material.two_sided);

        // Process Alpha Mode
        auto [Exist, AlphaString] = GetAIMaterialString(AIMaterial, AI_MATKEY_GLTF_ALPHAMODE);
        if (Exist) {
            if (AlphaString == "OPAQUE") {
                Material.alpha_mode = ALPHA_MODE_OPAQUE;
            } else if (AlphaString == "Mask") {
                GetAIMaterialFloat(AIMaterial, AI_MATKEY_GLTF_ALPHACUTOFF, Material.alpha_cutoff);
                Material.alpha_mode = ALPHA_MODE_TRANSPARENT;
            } else if (AlphaString == "Blend") {
                Material.alpha_mode = ALPHA_MODE_TRANSPARENT;
            }
        }

        auto RTScene = GetScene();

        // Process Normal Map
        auto Ret = GetAIMaterialTexturePath(AIMaterial, aiTextureType_NORMALS, 0);
        if (std::get<0>(Ret)) {
            std::string& SubPath = std::get<1>(Ret);
            std::filesystem::path Path = std::get<1>(Ret);
            const aiTexture* TextureData = m_aiScene->GetEmbeddedTexture(SubPath.c_str());
            if (TextureData) {
                auto [TexturePtr, Index] = RTScene->CreateOrRetrieveTexture(TextureData);
                Material.normal_tex_id = Index;
            } else {
                std::string FullPath = m_path.string() + '/' + SubPath;
                auto [TexturePtr, Index] = RTScene->CreateOrRetrieveTexture(FullPath);
                Material.normal_tex_id = Index;
            }
            Material.is_dds_normal_tex = Path.extension() == ".dds";
        }

        Ret = GetAIMaterialTexturePath(AIMaterial, aiTextureType_BASE_COLOR, 0);
        if (!std::get<0>(Ret)) {
            Ret = GetAIMaterialTexturePath(AIMaterial, aiTextureType_DIFFUSE, 0);  // For fbx format
        }
        if (std::get<0>(Ret)) {
            std::string& SubPath = std::get<1>(Ret);
            const aiTexture* TextureData = m_aiScene->GetEmbeddedTexture(SubPath.c_str());
            if (TextureData) {
                auto [TexturePtr, Index] = RTScene->CreateOrRetrieveTexture(TextureData);
                Material.albedo_tex_id = Index;
            } else {
                std::filesystem::path Path = std::get<1>(Ret);
                std::string FullPath = m_path.string() + '/' + SubPath;
                auto [TexturePtr, Index] = RTScene->CreateOrRetrieveTexture(FullPath);
                Material.albedo_tex_id = Index;
            }
        }

        Ret = GetAIMaterialTexturePath(AIMaterial, aiTextureType_GLTF_METALLIC_ROUGHNESS, 0);
        if (!std::get<0>(Ret)) {
            Ret = GetAIMaterialTexturePath(AIMaterial, aiTextureType_SPECULAR, 0);  // For fbx format
        }
        if (std::get<0>(Ret)) {
            std::string& SubPath = std::get<1>(Ret);
            const aiTexture* TextureData = m_aiScene->GetEmbeddedTexture(SubPath.c_str());
            if (TextureData) {
                auto [TexturePtr, Index] = RTScene->CreateOrRetrieveTexture(TextureData);
                Material.specular_tex_id = Index;
            } else {
                std::filesystem::path Path = std::get<1>(Ret);
                std::string FullPath = m_path.string() + '/' + SubPath;
                auto [TexturePtr, Index] = RTScene->CreateOrRetrieveTexture(FullPath);
                Material.specular_tex_id = Index;
            }
        }

        return Material;
    }

    void Model::GetAIMaterialColor(aiMaterial *AIMaterial, const char *pKey, unsigned int Type,
                                   unsigned int Idx, glm::vec3 &Value) {
        aiColor3D Temp;
        aiReturn Ret = AIMaterial->Get(pKey, Type, Idx, Temp);
        if (Ret == aiReturn_SUCCESS) {
            Value = {Temp.r, Temp.g, Temp.b};
        }
    }

    void Model::GetAIMaterialFloat(aiMaterial *AIMaterial, const char *pKey, unsigned int Type,
                                   unsigned int Idx, float &Value) {
        ai_real Temp;
        aiReturn Ret = AIMaterial->Get(pKey, Type, Idx, Temp);
        if (Ret == aiReturn_SUCCESS) {
            Value = Temp;
        }
    }

    void Model::GetAIMaterialInt(aiMaterial *AIMaterial, const char *pKey, unsigned int Type,
                                 unsigned int Idx, int &Value) {
        ai_int Temp;
        aiReturn Ret = AIMaterial->Get(pKey, Type, Idx, Temp);
        if (Ret == aiReturn_SUCCESS) {
            Value = Temp;
        }
    }

    auto Model::GetAIMaterialString(aiMaterial *AIMaterial, const char *pKey, unsigned int Type,
                                    unsigned int Idx) -> std::tuple<bool, std::string> {
        aiString Temp;
        aiReturn Ret = AIMaterial->Get(pKey, Type, Idx, Temp);
        std::string String;
        bool Exist = Ret == aiReturn_SUCCESS;
        if (Exist) {
            String = Temp.C_Str();
        }
        return {Exist, String};
    }

    auto Model::GetAIMaterialTexturePath(aiMaterial *AIMaterial, aiTextureType Type,
                                         uint Index) -> std::tuple<bool, std::string> {
        aiString Temp;
        aiReturn Ret = AIMaterial->GetTexture(Type, Index, &Temp);
        std::string String;
        bool Exist = Ret == aiReturn_SUCCESS;
        if (Exist) {
            String = Temp.C_Str();
        }
        return {Exist, String};
    }

} // namespace Shadowy
