//
// Created by HUSTLX on 2024/10/14.
//

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include "Model.h"
#include <unordered_map>


namespace HWPT {
    Model::Model(const std::filesystem::path &ModelPath,
                       const std::filesystem::path &TexturePath){
        LoadModel(ModelPath);
        m_texture = new Texture2D(TexturePath);
    }

    void Model::LoadModel(const std::filesystem::path &ModelPath) {
        tinyobj::attrib_t Attrib;
        std::vector<tinyobj::shape_t> Shapes;
        std::vector<tinyobj::material_t> Materials;
        std::string Warn, Err;

        bool LoadSuccess = tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err, ModelPath.string().c_str());
        Check(LoadSuccess);

        std::vector<Vertex> Vertices;
        std::vector<uint> Indices;
        std::unordered_map<Vertex, uint> UniqueVertices;

        Check(!Shapes.empty());
        for (const auto& Shape : Shapes) {
            Check(!Shape.mesh.indices.empty());
            for (const auto& Index : Shape.mesh.indices) {
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
}  // namespace HWPT
