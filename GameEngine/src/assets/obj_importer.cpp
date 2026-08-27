#include "obj_importer.hpp"
#include "debug/debug.hpp"

#include <filesystem>
#include <algorithm>
#include <cctype>

#define NOMINMAX
#define TINYOBJLOADER_DISABLE_FAST_FLOAT
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace Ludus::Assets
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
		// OBJ support is selected by file extension only for now
		std::string ext = std::filesystem::path(path).extension().string();

		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		return ext == ".obj";
	}

	bool ObjImporter::Import(const std::string& path, MeshImportData& outMesh)
	{
        // tinyobj stores shared attribute arrays plus per-face index triplets
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;
        const std::string materialDirectory =
            std::filesystem::path(path).parent_path().string();

        bool success = tinyobj::LoadObj(
            &attrib,
            &shapes,
            &materials,
            &warn,
            &err,
            path.c_str(),
            materialDirectory.c_str()
        );

        if (!warn.empty())
            Ludus::Debug::LogWarning(warn);

        if (!err.empty())
            Ludus::Debug::LogError(err);

        if (!success)
            return false;


        size_t totalVertices = 0;
        size_t totalIndices = 0;

		for (size_t shapeIndex = 0; shapeIndex < shapes.size(); ++shapeIndex)
		{
			const tinyobj::shape_t& shape = shapes[shapeIndex];
			MeshSurface surface;
			surface.name = shape.name.empty()
				? "surface_" + std::to_string(shapeIndex)
				: shape.name;
            std::unordered_map<ObjVertexKey, uint32_t, ObjVertexKeyHasher> uniqueVertices;

            for (const tinyobj::index_t& index : shape.mesh.indices)
            {
                if (index.vertex_index < 0)
                    continue;

                MeshVertex vertex{};

                // OBJ has separate indices for position, UV, and normal, so pack the used fields
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

                // use the full OBJ index triplet so UV seams and hard normals stay split
                ObjVertexKey key
                {
                    index.vertex_index,
                    index.texcoord_index,
                    index.normal_index
                };

                // reuse an engine vertex only when this exact triplet was already converted
                auto it = uniqueVertices.find(key);
                if (it != uniqueVertices.end())
                {
					surface.indices.push_back(it->second);
                    continue;
                }

				uint32_t newIndex = static_cast<uint32_t>(surface.vertices.size());
                uniqueVertices[key] = newIndex;

				surface.vertices.push_back(vertex);
				surface.indices.push_back(newIndex);
            }

			if (!surface.vertices.empty() && !surface.indices.empty())
			{
				totalVertices += surface.vertices.size();
				totalIndices += surface.indices.size();
				outMesh.surfaces.push_back(std::move(surface));
			}
		}

		if (!outMesh.surfaces.empty())
		{
			Ludus::Debug::LogVerbose("ObjImporter::Import : Imported ", path,
				" surfaces=", outMesh.surfaces.size(),
                " vertices=", totalVertices,
                " indices=", totalIndices);
        }

		return !outMesh.surfaces.empty();
	}
}
