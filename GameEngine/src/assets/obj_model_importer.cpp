#include "obj_model_importer.hpp"

#include <filesystem>
#include <algorithm>
#include <cctype>

#define NOMINMAX
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace Assets
{
	bool ObjModelImporter::CanLoad(const std::string& path) const
	{
		// OBJ support is selected by file extension only for now.
		std::string ext = std::filesystem::path(path).extension().string();

		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		return ext == ".obj";
	}

	bool ObjModelImporter::Load(const std::string& path, MeshSourceCollection& outModel)
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

        MeshSourceData mesh;

        for (const tinyobj::shape_t& shape : shapes)
        {
            // First pass combines all OBJ shapes into one engine mesh.
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

                mesh.vertices.push_back(vertex);
                mesh.indices.push_back(static_cast<uint32_t>(mesh.indices.size()));
            }
        }

        if (mesh.vertices.empty() || mesh.indices.empty())
            return false;

        outModel.meshes.push_back(std::move(mesh));
        return true;
	}
}
