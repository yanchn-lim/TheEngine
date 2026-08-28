#include "scene_asset_loader.hpp"

#include <algorithm>
#include <string_view>

#include "assets/asset_manager.hpp"
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

		bool LoadMaterials(
			const Ludus::Serialization::LSceneValue& category,
			Ludus::Assets::AssetManager& assets,
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
				if (!SceneValues::ValidateFields(*fields, { "source" }, {}, errors))
				{ success = false; continue; }
				const std::string* source =
					SceneValues::RequiredString(*fields, "source", value, errors);
				if (!source) { success = false; continue; }

				const Ludus::Assets::MaterialHandle handle =
					assets.LoadMaterialResource(*source);
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
			const Ludus::Serialization::LSceneValue& category,
			std::string_view sceneNamespace,
			Ludus::Assets::AssetManager& assets,
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

				const Ludus::Assets::MeshHandle handle = assets.LoadMesh(
					RegistryName(sceneNamespace, "mesh", name), *source);
				if (!handle)
				{
					AddError(errors, value, "failed to load mesh '" + name + "'");
					success = false;
					continue;
				}
				context.meshes.emplace(name, handle);

				const Ludus::Serialization::LSceneValue* assignments = FindField(*fields, "surface_materials");
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

				// component validation uses this result when no material
				// override is present.
				const Ludus::Assets::MeshAsset* mesh = assets.Get(handle);
				const bool hasDefaultMaterials = mesh && std::ranges::all_of(
					mesh->surfaces,
					[](const Ludus::Assets::MeshSurface& surface)
					{
						return surface.material.IsValid();
					});
				context.meshHasDefaultMaterials.emplace(name, hasDefaultMaterials);
			}
			return success;
		}
	}

	bool SceneAssetLoader::Load(
		const Ludus::Serialization::LSceneValue& root,
		std::string_view sceneNamespace,
		Ludus::Assets::AssetManager& assets,
		SceneAssetContext& context,
		std::vector<SceneLoadError>& errors)
	{
		const size_t firstError = errors.size();
		context = {};
		// publish aliases only after all declarations succeed. AssetManager
		// changes made during this load are not rolled back.
		SceneAssetContext loaded;
		const Ludus::Serialization::LSceneValue* assetsValue = root.Find("assets");
		if (!assetsValue)
			return true;

		const Object* categories = SceneValues::RequireObject(
			*assetsValue, "assets must be a block", errors);
		if (!categories)
			return false;

		bool success = true;
		// materials load first so mesh surfaces can resolve their aliases.
		for (const auto& [name, value] : *categories)
			if (name == "materials") success = LoadMaterials(value, assets, loaded, errors) && success;
		// mesh registry names include the scene namespace to avoid collisions.
		for (const auto& [name, value] : *categories)
			if (name == "meshes") success = LoadMeshes(
				value, sceneNamespace, assets, loaded, errors) && success;

		for (const auto& [name, value] : *categories)
		{
			if (name != "materials" && name != "meshes")
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
