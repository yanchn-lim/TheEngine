#include "asset_manager.hpp"

#include <filesystem>

#include "core/file_system.hpp"
#include "debug/debug.hpp"
#include "obj_importer.hpp"
#include "serialization/lscene_parser.hpp"

namespace
{
	std::string NormalizeResourcePath(const std::string& path)
	{
		return std::filesystem::path(path).lexically_normal().generic_string();
	}

	const std::string* ReadRequiredString(
		const Ludus::Serialization::LSceneValue& root,
		std::string_view field,
		const std::string& path)
	{
		const Ludus::Serialization::LSceneValue* value = root.Find(field);
		const std::string* text = value ? value->TryGetString() : nullptr;
		if (!text || text->empty())
			Ludus::Debug::LogError(path, ": field '", field, "' requires a non-empty string");
		return text && !text->empty() ? text : nullptr;
	}

	void ReadOptionalBoolean(
		const Ludus::Serialization::LSceneValue& root,
		std::string_view field,
		const std::string& path,
		bool& output)
	{
		const Ludus::Serialization::LSceneValue* value = root.Find(field);
		if (!value)
			return;
		const bool* boolean = value->TryGetBoolean();
		if (!boolean)
		{
			Ludus::Debug::LogError(
				path, ":", value->GetLocation().line,
				": field '", field, "' must be a boolean; using default");
			return;
		}
		output = *boolean;
	}

	void ReadOptionalBlendMode(
		const Ludus::Serialization::LSceneValue& root,
		const std::string& path,
		Ludus::Graphics::BlendMode& output)
	{
		const Ludus::Serialization::LSceneValue* value = root.Find("blend");
		if (!value)
			return;
		const std::string* text = value->TryGetString();
		if (!text)
		{
			Ludus::Debug::LogError(
				path, ":", value->GetLocation().line,
				": field 'blend' must be a string; using default");
			return;
		}

		if (*text == "none") output = Ludus::Graphics::BlendMode::NONE;
		else if (*text == "alpha") output = Ludus::Graphics::BlendMode::ALPHA;
		else if (*text == "additive") output = Ludus::Graphics::BlendMode::ADDITIVE;
		else if (*text == "premultiplied_alpha")
			output = Ludus::Graphics::BlendMode::PREMULTIPLIED_ALPHA;
		else if (*text == "multiply") output = Ludus::Graphics::BlendMode::MULTIPLY;
		else
		{
			Ludus::Debug::LogError(
				path, ":", value->GetLocation().line,
				": unknown blend mode '", *text, "'; using default");
		}
	}
}

namespace Ludus::Assets
{
	TextureHandle AssetManager::LoadTexture(const std::string& path)
	{
		// decode and register CPU texture data
		return _textures.Load(path);
	}

	TextureHandle AssetManager::CreateSolidColorTexture(
		const std::string& name,
		unsigned char red,
		unsigned char green,
		unsigned char blue,
		unsigned char alpha)
	{
		return _textures.CreateSolidColor(name, red, green, blue, alpha);
	}

	ShaderHandle AssetManager::LoadShader(const std::string& vertexPath, const std::string& fragmentPath,
		const std::string& vertexSpirvPath, const std::string& fragmentSpirvPath)
	{
		// store backend shader variants and deduplicate them by source paths
		return _shaders.Load(vertexPath, fragmentPath, vertexSpirvPath, fragmentSpirvPath);
	}

	ShaderHandle AssetManager::LoadShaderResource(const std::string& path)
	{
		const std::string normalizedPath = NormalizeResourcePath(path);
		if (const auto found = _shaderResources.find(normalizedPath);
			found != _shaderResources.end())
		{
			return found->second;
		}

		std::string source;
		if (!Ludus::FileSystem::ReadTextFile(normalizedPath.c_str(), source))
		{
			Ludus::Debug::LogError(
				"AssetManager::LoadShaderResource : Failed to read ", normalizedPath);
			return {};
		}

		const Ludus::Serialization::LSceneParseResult parsed =
			Ludus::Serialization::LSceneParser{}.Parse(source);
		if (!parsed)
		{
			for (const Ludus::Serialization::LSceneParseError& error : parsed.errors)
			{
				Ludus::Debug::LogError(
					normalizedPath, ":", error.location.line, ":",
					error.location.column, ": ", error.message);
			}
			return {};
		}

		if (parsed.resourceType != "shader")
		{
			Ludus::Debug::LogError(normalizedPath, ": expected shader resource");
			return {};
		}

		for (const auto& [field, value] : *parsed.root.TryGetObject())
		{
			if (field != "vertex" && field != "fragment" &&
				field != "vertex_spirv" && field != "fragment_spirv")
			{
				Ludus::Debug::LogError(
					normalizedPath, ":", value.GetLocation().line,
					": unknown shader field '", field, "'");
				return {};
			}
		}

		const std::string* vertex =
			ReadRequiredString(parsed.root, "vertex", normalizedPath);
		const std::string* fragment =
			ReadRequiredString(parsed.root, "fragment", normalizedPath);
		if (!vertex || !fragment)
			return {};

		std::string vertexSpirv;
		std::string fragmentSpirv;
		if (const Ludus::Serialization::LSceneValue* value = parsed.root.Find("vertex_spirv"))
		{
			const std::string* text = value->TryGetString();
			if (!text)
			{
				Ludus::Debug::LogError(
					normalizedPath, ": field 'vertex_spirv' must be a string");
				return {};
			}
			vertexSpirv = *text;
		}
		if (const Ludus::Serialization::LSceneValue* value = parsed.root.Find("fragment_spirv"))
		{
			const std::string* text = value->TryGetString();
			if (!text)
			{
				Ludus::Debug::LogError(
					normalizedPath, ": field 'fragment_spirv' must be a string");
				return {};
			}
			fragmentSpirv = *text;
		}

		const ShaderHandle handle =
			LoadShader(*vertex, *fragment, vertexSpirv, fragmentSpirv);
		if (handle)
			_shaderResources.emplace(normalizedPath, handle);
		return handle;
	}

	MaterialHandle AssetManager::LoadMaterialResource(const std::string& path)
	{
		const std::string normalizedPath = NormalizeResourcePath(path);
		if (const auto found = _materialResources.find(normalizedPath);
			found != _materialResources.end())
		{
			return found->second;
		}

		std::string source;
		if (!Ludus::FileSystem::ReadTextFile(normalizedPath.c_str(), source))
		{
			Ludus::Debug::LogError(
				"AssetManager::LoadMaterialResource : Failed to read ", normalizedPath);
			return {};
		}

		const Ludus::Serialization::LSceneParseResult parsed =
			Ludus::Serialization::LSceneParser{}.Parse(source);
		if (!parsed)
		{
			for (const Ludus::Serialization::LSceneParseError& error : parsed.errors)
			{
				Ludus::Debug::LogError(
					normalizedPath, ":", error.location.line, ":",
					error.location.column, ": ", error.message);
			}
			return {};
		}

		if (parsed.resourceType != "material")
		{
			Ludus::Debug::LogError(normalizedPath, ": expected material resource");
			return {};
		}

		for (const auto& [field, value] : *parsed.root.TryGetObject())
		{
			if (field != "shader" && field != "texture" &&
				field != "depth_test" && field != "depth_write" &&
				field != "blend" && field != "culling")
			{
				Ludus::Debug::LogError(
					normalizedPath, ":", value.GetLocation().line,
					": unknown material field '", field, "'; ignoring it");
			}
		}

		const std::string* shaderPath =
			ReadRequiredString(parsed.root, "shader", normalizedPath);
		const std::string* texturePath =
			ReadRequiredString(parsed.root, "texture", normalizedPath);
		if (!shaderPath || !texturePath)
			return {};

		const ShaderHandle shader = LoadShaderResource(*shaderPath);
		const TextureHandle texture = LoadTexture(NormalizeResourcePath(*texturePath));
		if (!shader || !texture)
		{
			Ludus::Debug::LogError(
				normalizedPath, ": failed to resolve material dependencies");
			return {};
		}

		Ludus::Graphics::RenderState state;
		ReadOptionalBoolean(parsed.root, "depth_test", normalizedPath, state.depthTest);
		ReadOptionalBoolean(parsed.root, "depth_write", normalizedPath, state.depthWrite);
		ReadOptionalBoolean(parsed.root, "culling", normalizedPath, state.culling);
		ReadOptionalBlendMode(parsed.root, normalizedPath, state.blendMode);

		const MaterialHandle handle =
			CreateMaterial(normalizedPath, shader, texture, state);
		if (handle)
			_materialResources.emplace(normalizedPath, handle);
		return handle;
	}

	MeshHandle AssetManager::LoadMesh(const std::string& name, const std::string& path)
	{
		MeshImportData mesh;
		if (!_modelImporters.Import(path, mesh))
		{
			Ludus::Debug::LogError("AssetManager::LoadMesh : Failed to load mesh from ", path);
			return {};
		}

		return _meshes.Create(name, mesh);
	}

	MaterialHandle AssetManager::CreateMaterial(const std::string& name, ShaderHandle shader, TextureHandle texture, Ludus::Graphics::RenderState state)
	{
		if (!Get(shader) || !Get(texture))
		{
			Ludus::Debug::LogError("AssetManager::CreateMaterial : Could not resolve shader or texture fallback for material ", name);
			return MaterialHandle{};
		}

		return _materials.Create(name, shader, texture, state);
	}

	MeshHandle AssetManager::CreateMesh(const std::string& name, const MeshSurface& surface)
	{
		// keep procedural mesh data in the asset registry until RenderResourceManager uploads it
		MeshImportData mesh;
		mesh.surfaces.push_back(surface);
		return _meshes.Create(name, mesh);
	}

	bool AssetManager::SetSurfaceMaterial(
		MeshHandle mesh,
		std::string_view surface,
		MaterialHandle material)
	{
		return Get(material) && _meshes.SetSurfaceMaterial(mesh, surface, material);
	}

	const TextureAsset* AssetManager::Get(TextureHandle handle) const
	{
		return _textures.Get(handle);
	}

	const ShaderAsset* AssetManager::Get(ShaderHandle handle) const
	{
		return _shaders.Get(handle);
	}

	const MaterialAsset* AssetManager::Get(MaterialHandle handle) const
	{
		return _materials.Get(handle);
	}

	const MaterialAsset* AssetManager::Get(const std::string& name) const
	{
		return _materials.Get(name);
	}

	const MeshAsset* AssetManager::Get(MeshHandle handle) const
	{
		return _meshes.Get(handle);
	}

	void AssetManager::Clear()
	{
		// clear dependent asset records before their referenced handles
		_meshes.Clear();
		_materialResources.clear();
		_materials.Clear();
		_shaderResources.clear();
		_shaders.Clear();
		_textures.Clear();
	}

	AssetManager::AssetManager()
	{
		// register format importers here so callers only talk to AssetManager
		_modelImporters.RegisterImporter(std::make_unique<ObjImporter>());
	}

}
