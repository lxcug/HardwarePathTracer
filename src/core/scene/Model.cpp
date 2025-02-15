//
// Created by HUSTLX on 2024/10/14.
//

#define TINYOBJLOADER_IMPLEMENTATION

#include <tiny_obj_loader.h>
#include "core/scene/Model.h"
#include <unordered_map>
#include "core/application/VulkanBackendApp.h"
#include "core/RHI.h"
#include "core/Utils.h"


namespace Shadowy {
    Model::Model(const std::filesystem::path &ModelPath,
                 const std::filesystem::path &TexturePath, bool GenerateMips)
            : m_generateMips(GenerateMips) {
        LoadModel(ModelPath);
        m_texture = new Texture2D(TexturePath, 1, m_generateMips);
    }

    Model::Model(const std::filesystem::path &ModelPath) {
        LoadModel(ModelPath);
    }

    void Model::LoadModel(const std::filesystem::path &ModelPath) {
        tinyobj::attrib_t Attrib;
        std::vector<tinyobj::shape_t> Shapes;
        std::vector<tinyobj::material_t> Materials;
        std::string Warn, Err;

        std::string MtlPath = ModelPath.parent_path().string();
        bool LoadSuccess = tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err,
                                            ModelPath.string().c_str(),
                                            MtlPath.c_str());
        Check(LoadSuccess);

        // Load Vertices Indices and Per Triangle Material Indices
        {
            std::vector<Vertex> Vertices;
            std::vector<uint> Indices;
            std::unordered_map<Vertex, uint> UniqueVertices;

            Check(!Shapes.empty());
            for (const auto &Shape: Shapes) {
                Check(!Shape.mesh.indices.empty());
                m_materialIndex.insert(m_materialIndex.end(), Shape.mesh.material_ids.begin(),
                                       Shape.mesh.material_ids.end());
                for (const auto &Index: Shape.mesh.indices) {
                    Vertex Vertex_{};
                    if (!Attrib.vertices.empty()) {
                        Vertex_.Pos = {
                                Attrib.vertices[3 * Index.vertex_index + 0],
                                Attrib.vertices[3 * Index.vertex_index + 1],
                                Attrib.vertices[3 * Index.vertex_index + 2]
                        };
                    }
                    if (!Attrib.normals.empty()) {
                        Vertex_.Normal = {
                                Attrib.normals[3 * Index.normal_index + 0],
                                Attrib.normals[3 * Index.normal_index + 1],
                                Attrib.normals[3 * Index.normal_index + 2]
                        };
                    }
                    if (!Attrib.texcoords.empty() && (2 * Index.texcoord_index + 1) < Attrib.texcoords.size()) {
                        Vertex_.TexCoord = {
                                Attrib.texcoords[2 * Index.texcoord_index + 0],
                                1.f - Attrib.texcoords[2 * Index.texcoord_index + 1]
                        };
                    }
                    if (!Attrib.colors.empty()) {
                        Vertex_.Color = {
                                Attrib.colors[3 * Index.vertex_index + 0],
                                Attrib.colors[3 * Index.vertex_index + 1],
                                Attrib.colors[3 * Index.vertex_index + 2]
                        };
                    }

                    if (UniqueVertices.find(Vertex_) == UniqueVertices.end()) {
                        UniqueVertices[Vertex_] = Vertices.size();
                        Vertices.push_back(Vertex_);
                    }

                    Indices.push_back(UniqueVertices[Vertex_]);
                }
            }

            m_vertexBuffer = new VertexBuffer(sizeof(Vertex) * Vertices.size(), Vertices.data());
            m_vertexBuffer->SetLayout({
                                              {VertexAttributeDataType::Float3, "Pos"},
                                              {VertexAttributeDataType::Float3, "Normal"},
                                              {VertexAttributeDataType::Float3, "Color"},
                                              {VertexAttributeDataType::Float2, "TexCoord"}
                                      });
            m_indexBuffer = new IndexBuffer(Indices.size(), Indices.data());
        }

        // Load Materials
        {
            for (const auto &Material: Materials) {
                struct Material Mat{};
                if (Material.diffuse) {
                    Mat.Albedo = {
                            Material.diffuse[0], Material.diffuse[1], Material.diffuse[2]
                    };
                }
                if (!Material.diffuse_texname.empty()) {
                    m_textureNames.push_back(MtlPath + '/' + Material.diffuse_texname);
                    Mat.AlbedoTextureID = static_cast<int>(m_textureNames.size()) - 1;
                }
                if (Material.specular) {
                    Mat.Specular = {
                            Material.specular[0], Material.specular[1], Material.specular[2]
                    };
                }
                if (!Material.specular_texname.empty()) {
                    m_textureNames.push_back(MtlPath + '/' + Material.specular_texname);
                    Mat.SpecularTextureID = static_cast<int>(m_textureNames.size()) - 1;
                }
                if (Material.emission) {
                    Mat.Emissive = {
                            Material.emission[0], Material.emission[1], Material.emission[2]
                    };
                }
                if (!Material.emissive_texname.empty()) {
                    m_textureNames.push_back(MtlPath + '/' + Material.emissive_texname);
                    Mat.EmissiveTextureID = static_cast<int>(m_textureNames.size()) - 1;
                }
                if (Material.transmittance) {
                    Mat.Transmittance = {
                            Material.transmittance[0], Material.transmittance[1],
                            Material.transmittance[2]
                    };
                }
                Mat.Opacity = Material.dissolve;
                Mat.Roughness = Material.roughness;
                Mat.Metallic = Material.metallic;
                if (!Material.roughness_texname.empty()) {
                    m_textureNames.push_back(MtlPath + '/' + Material.roughness_texname);
                    Mat.RoughnessTextureID = static_cast<int>(m_textureNames.size()) - 1;
                }
                if (!Material.metallic_texname.empty()) {
                    m_textureNames.push_back(MtlPath + '/' + Material.metallic_texname);
                    Mat.MetallicTextureID = static_cast<int>(m_textureNames.size()) - 1;
                }

                m_materials.push_back(Mat);
            }

            // Add Default Material
            if (m_materials.empty()) {
                m_materials.emplace_back();
            }

            // Fixing Material Indices
            for (auto &MatIndex: m_materialIndex) {
                if (MatIndex < 0 || MatIndex >= m_materials.size()) {
                    MatIndex = 0;
                }
            }
            m_materialBuffer = new ArbitraryBuffer(sizeof(Material) * m_materials.size(),
                                                   m_materials.data(),
                                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            m_materialIndexBuffer = new ArbitraryBuffer(sizeof(int) * m_materialIndex.size(),
                                                        m_materialIndex.data(),
                                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }

        m_modelDesc.VertexBufferAddress = RHI::GetBufferDeviceAddress(m_vertexBuffer->GetHandle());
        m_modelDesc.IndexBufferAddress = RHI::GetBufferDeviceAddress(m_indexBuffer->GetHandle());
        m_modelDesc.MaterialBufferAddress = RHI::GetBufferDeviceAddress(
                m_materialBuffer->GetHandle());
        m_modelDesc.MaterialIndexBufferAddress = RHI::GetBufferDeviceAddress(
                m_materialIndexBuffer->GetHandle());
    }

    Model::~Model() {
        delete m_texture;

        delete m_vertexBuffer;
        delete m_indexBuffer;
        delete m_materialBuffer;
        delete m_materialIndexBuffer;
    }

    void Model::Bind(VkCommandBuffer CommandBuffer) {
        m_vertexBuffer->Bind(CommandBuffer);
        m_indexBuffer->Bind(CommandBuffer);
    }

    void Model::DrawIndexed(VkCommandBuffer CommandBuffer) {
        this->Bind(CommandBuffer);
        vkCmdDrawIndexed(CommandBuffer, GetIndexCount(), 1, 0, 0, 0);
    }

    auto Model::GetBLASBuildInput() const -> BLASBuildInput {
        // Get Vertex/Index Buffer Device Address
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
        Triangles.transformData = {}; // Indicate Identity Transform

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

    auto
    Model::GetTLASBuildInput(ASBuilder *AccelBuilder) const -> VkAccelerationStructureInstanceKHR {
        VkAccelerationStructureInstanceKHR Instance{};
        Instance.transform = Utils::GLMToVulkanMatrix(m_transform);
        Instance.instanceCustomIndex = m_instanceID;
        Instance.accelerationStructureReference = AccelBuilder->GetBLASDeviceAddress(m_instanceID);
        Instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        Instance.mask = 0xff; //  Only be hit if rayMask & instance.mask != 0
        Instance.instanceShaderBindingTableRecordOffset = 0;

        return Instance;
    }
} // namespace Shadowy
