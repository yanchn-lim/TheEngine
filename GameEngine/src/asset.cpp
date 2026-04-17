#include "debug.hpp"

#include "asset.hpp"
#include "asset_types.hpp"

#include "mesh.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "material.hpp"
#include "vertex.hpp"

namespace
{
	Asset::Mesh MakeQuad()
	{
		// 1x1 square centered at origin. Scale to desired size via the model matrix.
		const std::vector<Vertex> verts =
		{
			{ float3(-0.5f,  0.5f, 0.f), float3(1.f, 1.f, 1.f) , float2(0.f, 1.f) },  // top-left
			{ float3(0.5f ,  0.5f, 0.f), float3(1.f, 1.f, 1.f) , float2(1.f, 1.f) },  // top-right
			{ float3(0.5f , -0.5f, 0.f), float3(1.f, 1.f, 1.f) , float2(1.f, 0.f) },  // bottom-right
			{ float3(-0.5f, -0.5f, 0.f), float3(1.f, 1.f, 1.f) , float2(0.f, 0.f) },  // bottom-left
		};

		// Two triangles: (top-left, top-right, bottom-right) and (bottom-right, bottom-left, top-left).
		const std::vector<uint> indices = { 0, 1, 2, 2, 3, 0 };

		Asset::Mesh m;
		m.Upload(verts, indices);
		return m;
	}

	Asset::Mesh MakeCircle(int segments)
	{
		std::vector<Vertex> verts;
		std::vector<uint> indices;

		verts.push_back({ float3(0.f, 0.f, 0.f), float3(1.f, 1.f, 1.f), float2(0.5f, 0.5f) }); // center vertex (hub of the fan)

		const float step = (2.f * glm::pi<float>()) / (float)segments;

		// Place one vertex per segment evenly around the circumference (radius 0.5).
		for (int i = 0; i < segments; ++i)
		{
			float angle = step * (float)i;
			float x = glm::cos(angle) * 0.5f;
			float y = glm::sin(angle) * 0.5f;

			verts.push_back(
				{
					float3(glm::cos(angle) * 0.5f, glm::sin(angle) * 0.5f,0.f),
					float3(1.f, 1.f, 1.f),
					float2(x + 0.5f, y + 0.5f)
				});
		}

		// Each triangle connects the center to two adjacent rim vertices.
		// Modulo wraps the last triangle back to vertex 1 to close the circle.
		for (int i = 1; i <= segments; ++i)
		{
			indices.push_back(0);
			indices.push_back((uint)i);
			indices.push_back((uint)(i % segments) + 1);
		}

		Asset::Mesh m;
		m.Upload(verts, indices);
		return m;
	}
}

namespace Asset
{
	std::unordered_map<AssetHandle, Mesh> _meshes;
	std::unordered_map<AssetHandle, Texture2D> _textures;
	std::unordered_map<AssetHandle, Shader> _shaders;
	std::unordered_map<AssetHandle, Material> _materials;

	void Initialize()
	{
		//look through entire asset folder for mesh files
		//upload to gpu
		//register
		Mesh quad = MakeQuad();
		Mesh circle = MakeCircle(32);
		LoadMesh("quad", quad);
		LoadMesh("circle", circle);

		//look through for shader files
		//compile and link
		//register
		//temp loading
		if (!LoadShader("unlit", "assets/unlit.vert", "assets/unlit.frag"))
		{
			Debug::CLog("Failed to load unlit shader\n");
		}

		//look through for textures
		//upload to gpu
		//register
		Texture2D tex{};
		tex.Upload("assets/wall.jpg");
		LoadTexture("wall_brick", tex);

		//create material
		Material mat{"unlit","wall_brick",0};
		LoadMaterial("unlit_mat", mat);

		Material mat_red{ "unlit", "wall_brick", 0 };
		LoadMaterial("unlit_mat_red", mat_red);
	}


	void Shutdown()
	{
		for (auto& [name, mesh] : _meshes)
			mesh.Shutdown();
		_meshes.clear();

		for (auto& [name, shader] : _shaders)
			shader.Shutdown();
		_shaders.clear();

		for (auto& [name, texture] : _textures)
			texture.Shutdown();
		_textures.clear();

		//for (auto& [name, material] : _materials)
			//material.Shutdown();
		_materials.clear();
	}

	bool LoadMesh(const AssetHandle& handle, Mesh& mesh)
	{
		//create asset handle
		if (HasMesh(handle))
		{
			Debug::LogWarning("Asset (Mesh) : ", handle, " already exists!\n");
			return true;
		}

		if (!mesh.IsValid())
		{
			Debug::LogError("Asset (Mesh) : ", handle, " has an invalid gpu handle!\n");
			return false;
		}

		_meshes.emplace(handle, mesh); // std::move transfers ownership without copying GPU handles
		Debug::CLog("Asset (Mesh) : ", handle, " loaded successfully!\n");
		return true;
	}

	bool LoadShader(const AssetHandle& handle, const std::string& vertPath, const std::string& fragPath)
	{
		if (HasShader(handle))
		{
			Debug::LogWarning("Asset (Shader) : ", handle, " already exists!\n");
			return true;
		}

		Shader shader(vertPath, fragPath);
		if (!shader.IsValid())
		{
			Debug::LogError("Asset (Shader) : ", handle, " has an invalid gpu handle!\n");
			return false;
		}

		_shaders.emplace(handle, std::move(shader));
		Debug::CLog("Asset (Shader) : ", handle, " loaded successfully!\n");
		return true;
	}

	bool LoadTexture(const AssetHandle& handle, Texture2D& tex)
	{
		if (HasTexture(handle))
		{
			Debug::LogWarning("Asset (Texture) : ", handle, " already exists!\n");
			return true;
		}

		if (!tex.IsValid())
		{
			Debug::LogError("Asset (Texture) : ", handle, " has an invalid gpu handle!\n");
			return false;
		}

		_textures.emplace(handle, std::move(tex)); // std::move transfers ownership without copying GPU handles
		Debug::CLog("Asset (Texture) : ", handle, " loaded successfully!\n");
		return true;
	}

	bool LoadMaterial(const AssetHandle& handle, Material& mat)
	{
		if (HasMaterial(handle))
		{
			Debug::LogWarning("Asset (Material) : ", handle, " already exists!\n");
			return true;
		}

		//if (!mat.IsValid())
		//{
		//	Debug::LogError("Asset (Texture) : ", handle, " has an invalid gpu handle!\n");
		//	return false;
		//}

		_materials.emplace(handle, std::move(mat)); // std::move transfers ownership without copying GPU handles
		Debug::CLog("Asset (Material) : ", handle, " loaded successfully!\n");
		return true;
	}

	bool HasMesh(const AssetHandle& handle)
	{
		return _meshes.count(handle) > 0;
	}

	bool HasShader(const AssetHandle& handle)
	{
		return _shaders.count(handle) > 0;
	}

	bool HasTexture(const AssetHandle& handle)
	{
		return _textures.count(handle) > 0;
	}

	bool HasMaterial(const AssetHandle& handle)
	{
		return _materials.count(handle) > 0;
	}

	Mesh& GetMesh(const AssetHandle& handle)
	{
		assert(_meshes.count(handle) && "Asset (Mesh) : asset not found!");
		return _meshes.at(handle);
	}

	Shader& GetShader(const AssetHandle& handle)
	{
		assert(_shaders.count(handle) && "Asset (Shader) : asset not found!");
		return _shaders.at(handle);
	}

	Texture2D& GetTexture(const AssetHandle& handle)
	{
		assert(_textures.count(handle) && "Asset (Texture) : asset not found!");
		return _textures.at(handle);
	}

	Material& GetMaterial(const AssetHandle& handle)
	{
		assert(_materials.count(handle) && "Asset (Material) : asset not found!");
		return _materials.at(handle);
	}
}