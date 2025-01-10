//
// Created by HUSTLX on 2024/10/14.
//

#define TINYOBJLOADER_IMPLEMENTATION

#include <tiny_obj_loader.h>
#include "Model.h"
#include <unordered_map>
#include "core/application/VulkanBackendApp.h"
#include "core/RHI.h"
#include "core/Utils.h"


namespace HWPT {
    Model::Model(const std::filesystem::path &ModelPath,
                 const std::filesystem::path &TexturePath, bool GenerateMips)
            : m_generateMips(GenerateMips) {
        LoadModel(ModelPath);
        m_texture = new Texture2D(TexturePath, 1, m_generateMips);
    }

    void Model::LoadModel(const std::filesystem::path &ModelPath) {
        tinyobj::attrib_t Attrib;
        std::vector<tinyobj::shape_t> Shapes;
        std::vector<tinyobj::material_t> Materials;
        std::string Warn, Err;

        bool LoadSuccess = tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err,
                                            ModelPath.string().c_str());
        Check(LoadSuccess);

        std::vector<Vertex> Vertices;
        std::vector<uint> Indices;
        std::unordered_map<Vertex, uint> UniqueVertices;

        Check(!Shapes.empty());
        for (const auto &Shape: Shapes) {
            Check(!Shape.mesh.indices.empty());
            for (const auto &Index: Shape.mesh.indices) {
                Vertex _Vertex{};
                _Vertex.Pos = {
                        Attrib.vertices[3 * Index.vertex_index + 0],
                        Attrib.vertices[3 * Index.vertex_index + 1],
                        Attrib.vertices[3 * Index.vertex_index + 2]
                };
                _Vertex.Color = {1.0f, 1.0f, 1.0f};
                _Vertex.TexCoord = {
                        Attrib.texcoords[2 * Index.texcoord_index + 0],
                        1.f - Attrib.texcoords[2 * Index.texcoord_index + 1]
                };

                if (UniqueVertices.find(_Vertex) == UniqueVertices.end()) {
                    UniqueVertices[_Vertex] = Vertices.size();
                    Vertices.push_back(_Vertex);
                }

                Indices.push_back(UniqueVertices[_Vertex]);
            }
        }

        m_vertexBuffer = new VertexBuffer(sizeof(Vertex) * Vertices.size(), Vertices.data());
        m_vertexBuffer->SetLayout({
                                          {VertexAttributeDataType::Float3, "Pos"},
                                          {VertexAttributeDataType::Float3, "Color"},
                                          {VertexAttributeDataType::Float2, "TexCoord"}
                                  });
        m_indexBuffer = new IndexBuffer(Indices.size(), Indices.data());
    }

    Model::~Model() {
        delete m_vertexBuffer;
        delete m_indexBuffer;
        delete m_texture;
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
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
        Triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        Triangles.vertexData.deviceAddress = VertexBufferAddress;
        Triangles.vertexStride = sizeof(Vertex);
        Triangles.maxVertex = m_vertexBuffer->GetVertexCount() - 1;
        Triangles.indexType = VK_INDEX_TYPE_UINT32;
        Triangles.indexData.deviceAddress = IndexBufferAddress;
        Triangles.transformData = {};  // Indicate Identity Transform

        // Create AS Geometry
        VkAccelerationStructureGeometryKHR ASGeometry{
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
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

    auto Model::GetTLASBuildInput() const -> VkAccelerationStructureInstanceKHR {
        VkAccelerationStructureInstanceKHR Instance{};
        Instance.transform = Utils::GLMToVulkanMatrix(m_transform);
        Instance.instanceCustomIndex = 0;
//        Instance.accelerationStructureReference
        Instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        Instance.mask = 0xff;  //  Only be hit if rayMask & instance.mask != 0
        Instance.instanceShaderBindingTableRecordOffset = 0;

        return Instance;
    }
}  // namespace HWPT
