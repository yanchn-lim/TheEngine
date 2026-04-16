#pragma once

#include "mesh.hpp"
#include "shader.hpp"
#include "texture.hpp"

namespace Graphics
{
	constexpr uint Hash(const char* str);

	struct AssetHandle
	{
		uint id{0}; //hashed id from a string
#ifdef _DEBUG
		std::string name{}; //for debugging and pinpointing missing assets
#endif


		bool IsValid() const { return id != 0; }
		bool operator==(const AssetHandle& o) const { return id == o.id; }
		bool operator!=(const AssetHandle& o) const { return !(id == o.id); }
		bool operator< (const AssetHandle& o) const { return id < o.id; }

		AssetHandle(const char* nm) : id(Hash(nm))
		{
	#ifdef _DEBUG
			name = nm;
	#endif
		}
	};

	namespace Resource
	{
		void Initialize();
		void Shutdown();

		bool LoadMesh(AssetHandle handle, Mesh& mesh);
		bool LoadShader(AssetHandle handle, const std::string& fragPath, const std::string& vertPath);
		bool LoadTexture(AssetHandle handle, Texture& tex);

		bool HasMesh(AssetHandle handle);
		bool HasShader(AssetHandle handle);
		bool HasTexture(AssetHandle handle);

		Mesh& GetMesh(AssetHandle handle);
		Shader& GetShader(AssetHandle handle);
		Texture& GetTexture(AssetHandle handle);
	}
}

namespace std
{
	//give std::hash a way to hash AssetHandle
	template<>
	struct hash<Graphics::AssetHandle>
	{
		size_t operator()(const Graphics::AssetHandle& h) const noexcept
		{
			return std::hash<uint>{}(h.id);
		}
	};
}

std::ostream& operator<< (std::ostream& os, Graphics::AssetHandle handle);
