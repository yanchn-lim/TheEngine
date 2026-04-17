#pragma once

namespace Asset
{
	//fwd decl
	class Mesh;
	class Texture2D;
	class Shader;
	class Material;
	struct AssetHandle;

	void Initialize();
	void Shutdown();

	//loaded-in assets
	bool LoadMesh(const AssetHandle& handle, Mesh& mesh);
	bool LoadShader(const AssetHandle& handle, const std::string& fragPath, const std::string& vertPath);
	bool LoadTexture(const AssetHandle& handle, Texture2D& tex);
	bool LoadMaterial(const AssetHandle& handle, Material& mat);

	bool HasMesh(const AssetHandle& handle);
	bool HasShader(const AssetHandle& handle);
	bool HasTexture(const AssetHandle& handle);
	bool HasMaterial(const AssetHandle& handle);

	Mesh& GetMesh(const AssetHandle& handle);
	Shader& GetShader(const AssetHandle& handle);
	Texture2D& GetTexture(const AssetHandle& handle);
	Material& GetMaterial(const AssetHandle& handle);
}