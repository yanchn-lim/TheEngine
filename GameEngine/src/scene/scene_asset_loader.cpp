#include "scene_asset_loader.hpp"

#include <algorithm>
#include <optional>
#include <string_view>

#include "assets/asset_manager.hpp"
#include "graphics/blend_mode.hpp"
#include "scene_value_reader.hpp"

namespace Ludus
{
	namespace
	{
		using SceneValues::Object;
		using SceneValues::AddError;
		using SceneValues::FindField;

		std::string RegistryName(
			std::string_view sceneNamespace,
			std::string_view category,
			std::string_view alias)
		{
			return std::string(sceneNamespace) + "::" +
				std::string(category) + "::" + std::string(alias);
		}

		std::optional<Graphics::BlendMode> ParseBlendMode(
			const Object& object,
			std::vector<SceneLoadError>& errors)
		{
			const Serialization::LSceneValue* value = FindField(object, "blend");
			if (!value)
				return Graphics::BlendMode::ALPHA;

			const std::string* text = value->TryGetString();
			if (!text)
			{
				AddError(errors, *value, "field 'blend' must be a string");
				return std::nullopt;
			}

			if (*text == "none") return Graphics::BlendMode::NONE;
			if (*text == "alpha") return Graphics::BlendMode::ALPHA;
			if (*text == "additive") return Graphics::BlendMode::ADDITIVE;
			if (*text == "premultiplied_alpha") return Graphics::BlendMode::PREMULTIPLIED_ALPHA;
			if (*text == "multiply") return Graphics::BlendMode::MULTIPLY;

			AddError(errors, *value, "unknown blend mode '" + *text + "'");
			return std::nullopt;
		}

		bool LoadShaders(
			const Serialization::LSceneValue& category,
			Assets::AssetManager& assets,
			SceneAssetContext& context,
			std::vector<SceneLoadError>& errors)
		{
			const Object* shaders = SceneValues::RequireObject(category, "shaders must be a block", errors);
			if (!shaders) return false;

			bool success = true;
			for (const auto& [name, value] : *shaders)
			{
				const Object* fields = SceneValues::RequireObject(
					value, name + " must be a block", errors);
				if (!fields) { success = false; continue; }
				if (!SceneValues::ValidateFields(
					*fields, { "vertex", "fragment", "vertex_spirv", "fragment_spirv" }, {}, errors))
				{ success = false; continue; }

				const std::string* vertex = SceneValues::RequiredString(*fields, "vertex", value, errors);
				const std::string* fragment = SceneValues::RequiredString(*fields, "fragment", value, errors);
				std::string vertexSpirv;
				std::string fragmentSpirv;
				const bool optionalValid =
					SceneValues::OptionalString(*fields, "vertex_spirv", vertexSpirv, errors) &&
					SceneValues::OptionalString(*fields, "fragment_spirv", fragmentSpirv, errors);
				if (!vertex || !fragment || !optionalValid) { success = false; continue; }

				const Assets::ShaderHandle handle = assets.LoadShader(*vertex, *fragment, vertexSpirv, fragmentSpirv);
				if (!handle)
				{
					AddError(errors, value, "failed to load shader '" + name + "'");
					success = false;
					continue;
				}
				context.shaders.emplace(name, handle);
			}
			return success;
		}

		bool LoadTextures(
			const Serialization::LSceneValue& category,
			Assets::AssetManager& assets,
			SceneAssetContext& context,
			std::vector<SceneLoadError>& errors)
		{
			const Object* textures = SceneValues::RequireObject(category, "textures must be a block", errors);
			if (!textures) return false;

			bool success = true;
			for (const auto& [name, value] : *textures)
			{
				const Object* fields = SceneValues::RequireObject(
					value, name + " must be a block", errors);
				if (!fields) { success = false; continue; }
				if (!SceneValues::ValidateFields(*fields, { "source" }, {}, errors))
				{ success = false; continue; }
				const std::string* source = SceneValues::RequiredString(*fields, "source", value, errors);
				if (!source) { success = false; continue; }

				const Assets::TextureHandle handle = assets.LoadTexture(*source);
				if (!handle)
				{
					AddError(errors, value, "failed to load texture '" + name + "'");
					success = false;
					continue;
				}
				context.textures.emplace(name, handle);
			}
			return success;
		}

		bool LoadMaterials(
			const Serialization::LSceneValue& category,
			std::string_view sceneNamespace,
			Assets::AssetManager& assets,
			SceneAssetContext& context,
			std::vector<SceneLoadError>& errors)
		{
			const Object* materials = SceneValues::RequireObject(category, "materials must be a block", errors);
			if (!materials) return false;

			bool success = true;
			for (const auto& [name, value] : *materials)
			{
				const Object* fields = SceneValues::RequireObject(
					value, name + " must be a block", errors);
				if (!fields) { success = false; continue; }
				if (!SceneValues::ValidateFields(
					*fields, { "shader", "texture", "depth_test", "depth_write", "blend", "culling" }, {}, errors))
				{ success = false; continue; }

				const std::string* shaderName = SceneValues::RequiredString(*fields, "shader", value, errors);
				const std::string* textureName = SceneValues::RequiredString(*fields, "texture", value, errors);
				const auto blend = ParseBlendMode(*fields, errors);
				if (!shaderName || !textureName || !blend) { success = false; continue; }

				const auto shader = context.shaders.find(*shaderName);
				const auto texture = context.textures.find(*textureName);
				if (shader == context.shaders.end())
				{
					AddError(errors, *FindField(*fields, "shader"), "unknown shader '" + *shaderName + "'");
					success = false;
					continue;
				}
				if (texture == context.textures.end())
				{
					AddError(errors, *FindField(*fields, "texture"), "unknown texture '" + *textureName + "'");
					success = false;
					continue;
				}

				Graphics::RenderState state;
				if (!SceneValues::OptionalBoolean(*fields, "depth_test", state.depthTest, errors) ||
					!SceneValues::OptionalBoolean(*fields, "depth_write", state.depthWrite, errors) ||
					!SceneValues::OptionalBoolean(*fields, "culling", state.culling, errors))
				{ success = false; continue; }
				state.blendMode = *blend;

				const Assets::MaterialHandle handle =
					assets.CreateMaterial(
						RegistryName(sceneNamespace, "material", name),
						shader->second,
						texture->second,
						state);
				if (!handle)
				{
					AddError(errors, value, "failed to create material '" + name + "'");
					success = false;
					continue;
				}
				context.materials.emplace(name, handle);
			}
			return success;
		}

		bool LoadMeshes(
			const Serialization::LSceneValue& category,
			std::string_view sceneNamespace,
			Assets::AssetManager& assets,
			SceneAssetContext& context,
			std::vector<SceneLoadError>& errors)
		{
			const Object* meshes = SceneValues::RequireObject(category, "meshes must be a block", errors);
			if (!meshes) return false;

			bool success = true;
			for (const auto& [name, value] : *meshes)
			{
				const Object* fields = SceneValues::RequireObject(
					value, name + " must be a block", errors);
				if (!fields) { success = false; continue; }
				if (!SceneValues::ValidateFields(*fields, { "source", "surface_materials" }, {}, errors))
				{ success = false; continue; }
				const std::string* source = SceneValues::RequiredString(*fields, "source", value, errors);
				if (!source) { success = false; continue; }

				const Assets::MeshHandle handle = assets.LoadMesh(
					RegistryName(sceneNamespace, "mesh", name), *source);
				if (!handle)
				{
					AddError(errors, value, "failed to load mesh '" + name + "'");
					success = false;
					continue;
				}
				context.meshes.emplace(name, handle);

				const Serialization::LSceneValue* assignments = FindField(*fields, "surface_materials");
				if (assignments)
				{
					const Object* assignmentFields = SceneValues::RequireObject(
						*assignments, "surface_materials must be a block", errors);
					if (!assignmentFields) { success = false; continue; }
					for (const auto& [surface, materialValue] : *assignmentFields)
					{
						const std::string* materialName = materialValue.TryGetString();
						if (!materialName)
						{
							AddError(errors, materialValue, "surface material must reference a material name");
							success = false;
							continue;
						}
						const auto material = context.materials.find(*materialName);
						if (material == context.materials.end())
						{
							AddError(errors, materialValue, "unknown material '" + *materialName + "'");
							success = false;
							continue;
						}
						if (!assets.SetSurfaceMaterial(handle, surface, material->second))
						{
							AddError(errors, materialValue, "unknown surface '" + surface + "'");
							success = false;
						}
					}
				}

				const Assets::MeshAsset* mesh = assets.Get(handle);
				const bool hasDefaultMaterials = mesh && std::ranges::all_of(
					mesh->surfaces,
					[](const Assets::MeshSurface& surface)
					{
						return surface.material.IsValid();
					});
				context.meshHasDefaultMaterials.emplace(name, hasDefaultMaterials);
			}
			return success;
		}
	}

	bool SceneAssetLoader::Load(
		const Serialization::LSceneValue& root,
		std::string_view sceneNamespace,
		Assets::AssetManager& assets,
		SceneAssetContext& context,
		std::vector<SceneLoadError>& errors)
	{
		const size_t firstError = errors.size();
		context = {};
		SceneAssetContext loaded;
		const Serialization::LSceneValue* assetsValue = root.Find("assets");
		if (!assetsValue)
			return true;

		const Object* categories = SceneValues::RequireObject(
			*assetsValue, "assets must be a block", errors);
		if (!categories)
			return false;

		bool success = true;
		for (const auto& [name, value] : *categories)
		{
			if (name == "shaders") success = LoadShaders(value, assets, loaded, errors) && success;
			else if (name == "textures") success = LoadTextures(value, assets, loaded, errors) && success;
		}
		for (const auto& [name, value] : *categories)
			if (name == "materials") success = LoadMaterials(
				value, sceneNamespace, assets, loaded, errors) && success;
		for (const auto& [name, value] : *categories)
			if (name == "meshes") success = LoadMeshes(
				value, sceneNamespace, assets, loaded, errors) && success;

		for (const auto& [name, value] : *categories)
		{
			if (name != "shaders" && name != "textures" && name != "materials" && name != "meshes")
			{
				AddError(errors, value, "unknown asset category '" + name + "'");
				success = false;
			}
		}
		if (!success || errors.size() != firstError)
			return false;

		context = std::move(loaded);
		return true;
	}
}
