#include "scene_loader.hpp"

#include <array>
#include <filesystem>

#include "assets/asset_manager.hpp"
#include "core/file_system.hpp"
#include "scene.hpp"
#include "scene_asset_loader.hpp"
#include "serialization/lscene_parser.hpp"

namespace Ludus
{
	namespace
	{
		using Object = Serialization::LSceneValue::Object;

		void AddError(
			std::vector<SceneLoadError>& errors,
			const Serialization::LSceneValue& value,
			std::string message)
		{
			errors.push_back({ std::move(message), value.GetLocation() });
		}

		bool ValidateRoot(
			const Serialization::LSceneValue& root,
			std::vector<SceneLoadError>& errors)
		{
			const Object* fields = root.TryGetObject();
			if (!fields) return false;

			bool valid = true;
			for (const auto& [name, value] : *fields)
			{
				if (name != "scene" && name != "version" && name != "assets" && name != "entities")
				{
					AddError(errors, value, "unknown scene field '" + name + "'");
					valid = false;
				}
			}
			return valid;
		}

		bool LoadEntities(
			const Serialization::LSceneValue& root,
			Scene& scene,
			const SceneAssetContext& assets,
			const SceneComponentRegistry& components,
			std::vector<SceneLoadError>& errors)
		{
			const Serialization::LSceneValue* entitiesValue = root.Find("entities");
			if (!entitiesValue)
			{
				AddError(errors, root, "scene requires an entities block");
				return false;
			}

			const Object* entities = entitiesValue->TryGetObject();
			if (!entities)
			{
				AddError(errors, *entitiesValue, "entities must be a block");
				return false;
			}

			bool success = true;
			for (const auto& [id, value] : *entities)
			{
				bool valid = true;
				const Object* fields = value.TryGetObject();
				if (!fields)
				{
					AddError(errors, value, "entity '" + id + "' must be a block");
					success = false;
					continue;
				}

				for (const auto& [field, fieldValue] : *fields)
				{
					if (field != "name" && field != "components")
					{
						AddError(errors, fieldValue, "unknown entity field '" + field + "'");
						valid = false;
					}
				}
				if (!valid)
				{
					success = false;
					continue;
				}

				std::string displayName = id;
				if (const auto nameField = fields->find("name"); nameField != fields->end())
				{
					const std::string* name = nameField->second.TryGetString();
					if (!name)
					{
						AddError(errors, nameField->second, "entity name must be a string");
						success = false;
						continue;
					}
					displayName = *name;
				}

				const auto componentField = fields->find("components");
				if (componentField == fields->end() || !componentField->second.TryGetObject())
				{
					AddError(errors, value, "entity '" + id + "' requires a components block");
					success = false;
					continue;
				}

				const ECS::Entity entity = scene.CreateEntity(id, std::move(displayName));
				if (!entity.IsValid())
				{
					AddError(errors, value, "duplicate entity id '" + id + "'");
					success = false;
					continue;
				}

				for (const auto& [componentName, componentValue] : *componentField->second.TryGetObject())
					success = components.Load(componentName, componentValue, assets,
						scene.GetWorld(), entity, errors) && success;
			}
			return success;
		}
	}

	bool SceneLoader::Load(
		const std::string& path,
		Scene& scene,
		Assets::AssetManager& assets,
		const SceneComponentRegistry& components,
		std::vector<SceneLoadError>& errors)
	{
		const size_t firstError = errors.size();
		const auto finish = [&](bool result)
		{
			for (size_t index = firstError; index < errors.size(); ++index)
				errors[index].path = path;
			return result;
		};

		if (scene.GetWorld().GetEntityCount() != 0)
		{
			errors.push_back({ "scene must be empty before loading", { 1, 1 } });
			return finish(false);
		}

		std::string source;
		if (!FileSystem::ReadTextFile(path.c_str(), source))
		{
			errors.push_back({ "failed to read scene file '" + path + "'", { 1, 1 } });
			return finish(false);
		}

		const Serialization::LSceneParseResult parsed = Serialization::LSceneParser{}.Parse(source);
		if (!parsed)
		{
			for (const Serialization::LSceneParseError& error : parsed.errors)
				errors.push_back({ error.message, error.location });
			return finish(false);
		}

		if (!ValidateRoot(parsed.root, errors))
			return finish(false);

		SceneAssetContext assetContext;
		const std::string sceneNamespace =
			std::filesystem::path(path).lexically_normal().generic_string();
		if (!SceneAssetLoader::Load(
			parsed.root, sceneNamespace, assets, assetContext, errors))
			return finish(false);

		Scene stagedScene;
		if (!LoadEntities(parsed.root, stagedScene, assetContext, components, errors))
			return finish(false);

		scene.Swap(stagedScene);
		return finish(errors.size() == firstError);
	}
}
