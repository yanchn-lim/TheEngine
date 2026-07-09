#include "obj_importer.hpp"

#include <filesystem>
#include <algorithm>
#include <cctype>

#define NOMINMAX
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace Assets
{
    namespace
    {
        struct ObjVertexKey
        {
            int vertexIndex;
            int texcoordIndex;
            int normalIndex;

            bool operator==(const ObjVertexKey& other) const
            {
                return vertexIndex == other.vertexIndex
                    && texcoordIndex == other.texcoordIndex
                    && normalIndex == other.normalIndex;
            }
        };

        struct ObjVertexKeyHasher
        {
            size_t operator()(const ObjVertexKey& key) const
            {
                size_t h1 = std::hash<int>{}(key.vertexIndex);
                size_t h2 = std::hash<int>{}(key.texcoordIndex);
                size_t h3 = std::hash<int>{}(key.normalIndex);

                return h1 ^ (h2 << 1) ^ (h3 << 2);
            }
        };
    }

	bool ObjImporter::CanImport(const std::string& path) const
	{
		// OBJ support is selected by file extension only for now.
		std::string ext = std::filesystem::path(path).extension().string();

		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		return ext == ".obj";
	}

	bool ObjImporter::Import(const std::string& path, ModelImportData& outModel)
	{
        // tinyobj stores shared attribute arrays plus per-face index triplets.
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;

        bool success = tinyobj::LoadObj(
            &attrib,
            &shapes,
            &materials,
            &warn,
            &err,
            path.c_str()
        );

        if (!warn.empty())
            Debug::LogWarning(warn);

        if (!err.empty())
            Debug::LogError(err);

        if (!success)
            return false;


        size_t totalVertices = 0;
        size_t totalIndices = 0;

        for (const tinyobj::shape_t& shape : shapes)
        {
            MeshImportData mesh;
            std::unordered_map<ObjVertexKey, uint32_t, ObjVertexKeyHasher> uniqueVertices;

            for (const tinyobj::index_t& index : shape.mesh.indices)
            {
                if (index.vertex_index < 0)
                    continue;

                MeshVertex vertex{};

                // OBJ has separate indices for position/uv/normal; pack the used fields.
                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.color = { 1.0f, 1.0f, 1.0f };

                if (index.texcoord_index >= 0)
                {
                    vertex.texCoord0 = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                }

                // Use the full OBJ index triplet so UV seams and hard normals stay split.
                ObjVertexKey key
                {
                    index.vertex_index,
                    index.texcoord_index,
                    index.normal_index
                };

                // Reuse an engine vertex only when this exact triplet was already converted.
                auto it = uniqueVertices.find(key);
                if (it != uniqueVertices.end())
                {
                    mesh.indices.push_back(it->second);
                    continue;
                }

                uint32_t newIndex = static_cast<uint32_t>(mesh.vertices.size());
                uniqueVertices[key] = newIndex;

                mesh.vertices.push_back(vertex);
                mesh.indices.push_back(newIndex);
            }

            if (!mesh.vertices.empty() && !mesh.indices.empty())
            {
                totalVertices += mesh.vertices.size();
                totalIndices += mesh.indices.size();
                outModel.meshes.push_back(std::move(mesh));
            }
        }

        if (!outModel.meshes.empty())
        {
            Debug::LogVerbose("ObjImporter::Import : Imported ", path,
                " meshes=", outModel.meshes.size(),
                " vertices=", totalVertices,
                " indices=", totalIndices);
        }

        return !outModel.meshes.empty();
	}
}
