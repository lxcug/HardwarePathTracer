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
        std::vector<int> MaterialIndices(AIMesh->mNumFaces);

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
            MaterialIndices[i] = AIMesh->mMaterialIndex;
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
        m_materialIndexBuffer = new ArbitraryBuffer(
            sizeof(int) * MaterialIndices.size(),
            MaterialIndices.data(),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        );
        m_meshDesc.MaterialIndexBufferAddress = RHI::GetBufferDeviceAddress(m_materialIndexBuffer->GetHandle());
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
        ASGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

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

        int UpAxis = -1, FrontAxis = -1, RightAxis = -1;
        int UpSign = 0, FrontSign = 0, RightSign = 0;
        m_aiScene->mMetaData->Get("UpAxis", UpAxis);
        m_aiScene->mMetaData->Get("FrontAxis", FrontAxis);
        m_aiScene->mMetaData->Get("CoordAxis", RightAxis);
        m_aiScene->mMetaData->Get("UpAxisSign", UpSign);
        m_aiScene->mMetaData->Get("FrontAxisSign", FrontSign);
        m_aiScene->mMetaData->Get("CoordAxisSign", RightSign);

        auto IdentityTransform = glm::identity<glm::mat4>();
        ProcessNode(m_aiScene->mRootNode, m_aiScene, IdentityTransform);

        m_materialBuffer = new ArbitraryBuffer(
            sizeof(Material) * m_materials.size(),
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
            m_materials[MaterialIndex] = ProcessMaterial(Scene->mMaterials[MaterialIndex], m_aiScene);
        }

        for (uint i = 0; i < Node->mNumChildren; i++)
        {
            ProcessNode(Node->mChildren[i], Scene, CurrentNodeTransform);
        }
    }

    auto Model::ProcessMaterial(aiMaterial* AIMaterial, const aiScene* Scene) -> Material
    {
        // for (unsigned int i = 0; i < AIMaterial->mNumProperties; ++i) {
        //     const aiMaterialProperty* property = AIMaterial->mProperties[i];
        //     std::cout << "Property Key: " << property->mKey.C_Str() << std::endl;
        //     std::cout << "Semantic: " << property->mSemantic << std::endl;
        //     std::cout << "Index: " << property->mIndex << std::endl;
        //     std::cout << "Data Length: " << property->mDataLength << std::endl;
        //     std::cout << "---------------------" << std::endl;
        // }

        aiReturn AIRet{};
        Material Mat;
        aiColor3D Albedo, Emissive, Transmittance;
        AIRet = AIMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, Albedo);
        if (AIRet == aiReturn_SUCCESS)
        {
            Mat.Albedo = {Albedo.r, Albedo.g, Albedo.b};
        }
        AIRet = AIMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, Emissive);
        if (AIRet == aiReturn_SUCCESS)
        {
            Mat.Emissive = {Emissive.r, Emissive.g, Emissive.b};
        }
        AIRet = AIMaterial->Get(AI_MATKEY_COLOR_TRANSPARENT, Transmittance);
        if (AIRet == aiReturn_SUCCESS)
        {
            Mat.Transmittance = {Transmittance.r, Transmittance.g, Transmittance.b};
        }
        ai_real Roughness, Metallic, Opacity, IOR;
        AIRet = AIMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, Roughness);
        if (AIRet == aiReturn_SUCCESS)
        {
            Mat.Roughness = Roughness;
        }
        AIRet = AIMaterial->Get(AI_MATKEY_REFRACTI, IOR);
        if (AIRet == aiReturn_SUCCESS)
        {
            Mat.IOR = IOR;
        }
        AIRet = AIMaterial->Get(AI_MATKEY_METALLIC_FACTOR, Metallic);
        if (AIRet == aiReturn_SUCCESS)
        {
            Mat.Metallic = Metallic;
        }
        AIRet = AIMaterial->Get(AI_MATKEY_OPACITY, Opacity);
        if (AIRet == aiReturn_SUCCESS)
        {
            Mat.Opacity = Opacity;
        }

        auto RTScene = GetScene();
        // Process Texture
        if (AIMaterial->GetTextureCount(aiTextureType_DIFFUSE) == 1)
        {
            aiString TexturePath;
            AIMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &TexturePath);
            std::filesystem::path Path = TexturePath.C_Str();
            const aiTexture* TextureData = m_aiScene->GetEmbeddedTexture(TexturePath.C_Str());
            std::tuple<Texture2D*, uint> Ret;
            if (TextureData)
            {
                Ret = RTScene->CreateOrRetrieveTexture(TextureData);
            }
            else
            {
                std::string FullPath = m_path.string() + '/' + TexturePath.C_Str();
                Ret = RTScene->CreateOrRetrieveTexture(FullPath);
            }
            Mat.AlbedoTextureID = std::get<1>(Ret);
        }
        if (AIMaterial->GetTextureCount(aiTextureType_SHININESS) == 1)
        {
            aiString TexturePath;
            AIMaterial->GetTexture(aiTextureType_SHININESS, 0, &TexturePath);
            std::filesystem::path Path = TexturePath.C_Str();
            const aiTexture* TextureData = m_aiScene->GetEmbeddedTexture(TexturePath.C_Str());
            std::tuple<Texture2D*, uint> Ret;
            if (TextureData)
            {
                Ret = RTScene->CreateOrRetrieveTexture(TextureData);
            }
            else
            {
                std::string FullPath = m_path.string() + '/' + TexturePath.C_Str();
                Ret = RTScene->CreateOrRetrieveTexture(FullPath);
            }
            Mat.RoughnessTextureID = std::get<1>(Ret);
        }
        if (AIMaterial->GetTextureCount(aiTextureType_NORMALS) == 1)
        {
            aiString TexturePath;
            AIMaterial->GetTexture(aiTextureType_NORMALS, 0, &TexturePath);
            std::filesystem::path Path = TexturePath.C_Str();
            const aiTexture* TextureData = m_aiScene->GetEmbeddedTexture(TexturePath.C_Str());
            std::tuple<Texture2D*, uint> Ret;
            if (TextureData)
            {
                Ret = RTScene->CreateOrRetrieveTexture(TextureData);
            }
            else
            {
                std::string FullPath = m_path.string() + '/' + TexturePath.C_Str();
                Ret = RTScene->CreateOrRetrieveTexture(FullPath);
            }
            Mat.NormalTextureID = std::get<1>(Ret);
            Mat.IsDDSNormalTexture = Path.extension().string() == ".dds";
        }
        if (AIMaterial->GetTextureCount(aiTextureType_SPECULAR) == 1)
        {
            aiString TexturePath;
            AIMaterial->GetTexture(aiTextureType_SPECULAR, 0, &TexturePath);
            std::filesystem::path Path = TexturePath.C_Str();
            const aiTexture* TextureData = m_aiScene->GetEmbeddedTexture(TexturePath.C_Str());
            std::tuple<Texture2D*, uint> Ret;
            if (TextureData)
            {
                Ret = RTScene->CreateOrRetrieveTexture(TextureData);
            }
            else
            {
                std::string FullPath = m_path.string() + '/' + TexturePath.C_Str();
                Ret = RTScene->CreateOrRetrieveTexture(FullPath);
            }
            Mat.SpecularTextureID = std::get<1>(Ret);
        }
        if (AIMaterial->GetTextureCount(aiTextureType_EMISSION_COLOR) == 1)
        {
            aiString TexturePath;
            AIMaterial->GetTexture(aiTextureType_EMISSION_COLOR, 0, &TexturePath);
            const aiTexture* TextureData = m_aiScene->GetEmbeddedTexture(TexturePath.C_Str());
            std::tuple<Texture2D*, uint> Ret;
            if (TextureData)
            {
                Ret = RTScene->CreateOrRetrieveTexture(TextureData);
            }
            else
            {
                std::string FullPath = m_path.string() + '/' + TexturePath.C_Str();
                Ret = RTScene->CreateOrRetrieveTexture(FullPath);
            }
            Mat.EmissiveTextureID = std::get<1>(Ret);
        }
        if (AIMaterial->GetTextureCount(aiTextureType_METALNESS) == 1)
        {
            aiString TexturePath;
            AIMaterial->GetTexture(aiTextureType_METALNESS, 0, &TexturePath);
            std::filesystem::path Path = TexturePath.C_Str();
            const aiTexture* TextureData = m_aiScene->GetEmbeddedTexture(TexturePath.C_Str());
            std::tuple<Texture2D*, uint> Ret;
            if (TextureData)
            {
                Ret = RTScene->CreateOrRetrieveTexture(TextureData);
            }
            else
            {
                std::string FullPath = m_path.string() + '/' + TexturePath.C_Str();
                Ret = RTScene->CreateOrRetrieveTexture(FullPath);
            }
            Mat.MetallicTextureID = std::get<1>(Ret);
        }

        return Mat;
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

    // ObjModel::ObjModel(const std::filesystem::path &ModelPath) {
    //     LoadModel(ModelPath);
    // }
    //
    // void ObjModel::LoadModel(const std::filesystem::path &ModelPath) {
    //     tinyobj::attrib_t Attrib;
    //     std::vector<tinyobj::shape_t> Shapes;
    //     std::vector<tinyobj::material_t> Materials;
    //     std::string Warn, Err;
    //
    //     std::string MtlPath = ModelPath.parent_path().string();
    //     bool LoadSuccess = tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err,
    //                                         ModelPath.string().c_str(),
    //                                         MtlPath.c_str());
    //     Check(LoadSuccess);
    //
    //     // Load Vertices Indices and Per Triangle Material Indices
    //     {
    //         std::vector<Vertex> Vertices;
    //         std::vector<uint> Indices;
    //         std::unordered_map<Vertex, uint> UniqueVertices;
    //
    //         Check(!Shapes.empty());
    //         for (const auto &Shape: Shapes) {
    //             Check(!Shape.mesh.indices.empty());
    //             m_materialIndex.insert(m_materialIndex.end(), Shape.mesh.material_ids.begin(),
    //                                    Shape.mesh.material_ids.end());
    //             for (const auto &Index: Shape.mesh.indices) {
    //                 Vertex Vertex_{};
    //                 if (!Attrib.vertices.empty()) {
    //                     Vertex_.Pos = {
    //                             Attrib.vertices[3 * Index.vertex_index + 0],
    //                             Attrib.vertices[3 * Index.vertex_index + 1],
    //                             Attrib.vertices[3 * Index.vertex_index + 2]
    //                     };
    //                 }
    //                 if (!Attrib.normals.empty()) {
    //                     Vertex_.Normal = {
    //                             Attrib.normals[3 * Index.normal_index + 0],
    //                             Attrib.normals[3 * Index.normal_index + 1],
    //                             Attrib.normals[3 * Index.normal_index + 2]
    //                     };
    //                 }
    //                 if (!Attrib.texcoords.empty() && (2 * Index.texcoord_index + 1) < Attrib.texcoords.size()) {
    //                     Vertex_.TexCoord = {
    //                             Attrib.texcoords[2 * Index.texcoord_index + 0],
    //                             1.f - Attrib.texcoords[2 * Index.texcoord_index + 1]
    //                     };
    //                 }
    //                 if (!Attrib.colors.empty()) {
    //                     Vertex_.Color = {
    //                             Attrib.colors[3 * Index.vertex_index + 0],
    //                             Attrib.colors[3 * Index.vertex_index + 1],
    //                             Attrib.colors[3 * Index.vertex_index + 2]
    //                     };
    //                 }
    //
    //                 if (UniqueVertices.find(Vertex_) == UniqueVertices.end()) {
    //                     UniqueVertices[Vertex_] = Vertices.size();
    //                     Vertices.push_back(Vertex_);
    //                 }
    //
    //                 Indices.push_back(UniqueVertices[Vertex_]);
    //             }
    //         }
    //
    //         m_vertexBuffer = new VertexBuffer(sizeof(Vertex) * Vertices.size(), Vertices.data());
    //         m_vertexBuffer->SetLayout({
    //                                           {VertexAttributeDataType::Float3, "Pos"},
    //                                           {VertexAttributeDataType::Float3, "Normal"},
    //                                           {VertexAttributeDataType::Float3, "Color"},
    //                                           {VertexAttributeDataType::Float2, "TexCoord"},
    //                                           {VertexAttributeDataType::Float3, "Tangent"},
    //                                           {VertexAttributeDataType::Float3, "Bitangent"}
    //                                   });
    //         m_indexBuffer = new IndexBuffer(Indices.size(), Indices.data());
    //     }
    //
    //     // Load Materials
    //     {
    //         for (const auto &Material: Materials) {
    //             struct Material Mat{};
    //             if (Material.diffuse) {
    //                 Mat.Albedo = {
    //                         Material.diffuse[0], Material.diffuse[1], Material.diffuse[2]
    //                 };
    //             }
    //             if (!Material.diffuse_texname.empty()) {
    //                 m_textureNames.push_back(MtlPath + '/' + Material.diffuse_texname);
    //                 Mat.AlbedoTextureID = static_cast<int>(m_textureNames.size()) - 1;
    //             }
    //             if (Material.emission) {
    //                 Mat.Emissive = {
    //                         Material.emission[0], Material.emission[1], Material.emission[2]
    //                 };
    //             }
    //             if (!Material.emissive_texname.empty()) {
    //                 m_textureNames.push_back(MtlPath + '/' + Material.emissive_texname);
    //                 Mat.EmissiveTextureID = static_cast<int>(m_textureNames.size()) - 1;
    //             }
    //             if (Material.transmittance) {
    //                 Mat.Transmittance = {
    //                         Material.transmittance[0], Material.transmittance[1],
    //                         Material.transmittance[2]
    //                 };
    //             }
    //             Mat.Opacity = Material.dissolve;
    //             Mat.Roughness = Material.roughness;
    //             Mat.Metallic = Material.metallic;
    //             if (!Material.roughness_texname.empty()) {
    //                 m_textureNames.push_back(MtlPath + '/' + Material.roughness_texname);
    //                 Mat.RoughnessTextureID = static_cast<int>(m_textureNames.size()) - 1;
    //             }
    //             if (!Material.metallic_texname.empty()) {
    //                 m_textureNames.push_back(MtlPath + '/' + Material.metallic_texname);
    //                 Mat.MetallicTextureID = static_cast<int>(m_textureNames.size()) - 1;
    //             }
    //
    //             m_materials.push_back(Mat);
    //         }
    //
    //         // Add Default Material
    //         if (m_materials.empty()) {
    //             m_materials.emplace_back();
    //         }
    //
    //         // Fixing Material Indices
    //         for (auto &MatIndex: m_materialIndex) {
    //             if (MatIndex < 0 || MatIndex >= m_materials.size()) {
    //                 MatIndex = 0;
    //             }
    //         }
    //         m_materialBuffer = new ArbitraryBuffer(sizeof(Material) * m_materials.size(),
    //                                                m_materials.data(),
    //                                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
    //                                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    //                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    //         m_materialIndexBuffer = new ArbitraryBuffer(sizeof(int) * m_materialIndex.size(),
    //                                                     m_materialIndex.data(),
    //                                                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
    //                                                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    //                                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    //     }
    //
    //     m_modelDesc.VertexBufferAddress = RHI::GetBufferDeviceAddress(m_vertexBuffer->GetHandle());
    //     m_modelDesc.IndexBufferAddress = RHI::GetBufferDeviceAddress(m_indexBuffer->GetHandle());
    //     m_modelDesc.MaterialBufferAddress = RHI::GetBufferDeviceAddress(
    //             m_materialBuffer->GetHandle());
    //     m_modelDesc.MaterialIndexBufferAddress = RHI::GetBufferDeviceAddress(
    //             m_materialIndexBuffer->GetHandle());
    // }
    //
    // ObjModel::~ObjModel() {
    //     delete m_vertexBuffer;
    //     delete m_indexBuffer;
    //     delete m_materialBuffer;
    //     delete m_materialIndexBuffer;
    // }
    //
    // void ObjModel::Bind(VkCommandBuffer CommandBuffer) {
    //     m_vertexBuffer->Bind(CommandBuffer);
    //     m_indexBuffer->Bind(CommandBuffer);
    // }
    //
    // void ObjModel::DrawIndexed(VkCommandBuffer CommandBuffer) {
    //     this->Bind(CommandBuffer);
    //     vkCmdDrawIndexed(CommandBuffer, GetIndexCount(), 1, 0, 0, 0);
    // }
    //
    // auto ObjModel::GetBLASBuildInput() const -> BLASBuildInput {
    //     // Get Vertex/Index Buffer Device Address
    //     VkDeviceAddress VertexBufferAddress = RHI::GetBufferDeviceAddress(
    //             m_vertexBuffer->GetHandle());
    //     VkDeviceAddress IndexBufferAddress = RHI::GetBufferDeviceAddress(
    //             m_indexBuffer->GetHandle());
    //
    //     VkAccelerationStructureGeometryTrianglesDataKHR Triangles{
    //             VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR
    //     };
    //     Triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    //     Triangles.vertexData.deviceAddress = VertexBufferAddress;
    //     Triangles.vertexStride = sizeof(Vertex);
    //     Triangles.maxVertex = m_vertexBuffer->GetVertexCount() - 1;
    //     Triangles.indexType = VK_INDEX_TYPE_UINT32;
    //     Triangles.indexData.deviceAddress = IndexBufferAddress;
    //     Triangles.transformData = {};  // Indicate Identity Transform
    //
    //     // Create AS Geometry
    //     VkAccelerationStructureGeometryKHR ASGeometry{
    //             VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR
    //     };
    //     ASGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    //     ASGeometry.geometry.triangles = Triangles;
    //     ASGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    //
    //     // Create AS Build Range
    //     VkAccelerationStructureBuildRangeInfoKHR BuildRange;
    //     BuildRange.primitiveCount = m_indexBuffer->GetIndexCount() / 3;
    //     BuildRange.primitiveOffset = 0;
    //     BuildRange.firstVertex = 0;
    //     BuildRange.transformOffset = 0;
    //
    //     BLASBuildInput Input;
    //     Input.ASGeometries.emplace_back(ASGeometry);
    //     Input.ASBuildRangeInfos.emplace_back(BuildRange);
    //
    //     return Input;
    // }
    //
    // auto
    // ObjModel::GetTLASBuildInput(const ASBuilder *AccelBuilder) const -> VkAccelerationStructureInstanceKHR {
    //     VkAccelerationStructureInstanceKHR Instance{};
    //     Instance.transform = Utils::GLMToVulkanMatrix(m_transform);
    //     Instance.instanceCustomIndex = m_instanceID;
    //     Instance.accelerationStructureReference = AccelBuilder->GetBLASDeviceAddress(m_instanceID);
    //     Instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    //     Instance.mask = 0xff; //  Only be hit if rayMask & instance.mask != 0
    //     Instance.instanceShaderBindingTableRecordOffset = 0;
    //
    //     return Instance;
    // }

} // namespace Shadowy
