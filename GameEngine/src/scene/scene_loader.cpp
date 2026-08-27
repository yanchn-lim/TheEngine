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
		using Object = Ludus::Serialization::LSceneValue::Object;

		void AddError(
			std::vector<SceneLoadError>& errors,
			const Ludus::Serialization::LSceneValue& value,
			std::string message)
		{
			errors.push_back({ std::move(message), value.GetLocation() });
		}

		bool ValidateRoot(
			const Ludus::Serialization::LSceneValue& root,
			std::vector<SceneLoadError>& errors)
		{
			const Object* fields = root.TryGetObject();
			if (!fields) return false;

			bool valid = true;
			for (const auto& [name, value] : *fields)
			{
				if (name != "scene" && name != "version" && name != "assets" &&
					name != "systems" && name != "entities")
				{
					AddError(errors, value, "unknown scene field '" + name + "'");
					valid = false;
				}
			}
			return valid;
		}

		bool LoadSystemDefinitions(
			const Ludus::Serialization::LSceneValue& root,
			const SystemRegistry& registry,
			std::vector<SceneSystemDefinition>& definitions,
			std::vector<SceneLoadError>& errors)
		{
			const Ludus::Serialization::LSceneValue* systemsValue = root.Find("systems");
			if (!systemsValue)
				return true;

			const Object* systems = systemsValue->TryGetObject();
			if (!systems)
			{
				AddError(errors, *systemsValue, "systems must be a block");
				return false;
			}

			bool success = true;
			for (const auto& [id, value] : *systems)
			{
				const Object* fields = value.TryGetObject();
				if (!fields)
				{
					AddError(errors, value, "system '" + id + "' must be a block");
					success = false;
					continue;
				}

				bool fieldsValid = true;
				for (const auto& [name, fieldValue] : *fields)
				{
					if (name != "enabled" && name != "config")
					{
						AddError(errors, fieldValue, "unknown system field '" + name + "'");
						fieldsValid = false;
					}
				}

				const auto enabledField = fields->find("enabled");
				if (enabledField == fields->end())
				{
					AddError(errors, value, "system '" + id + "' requires enabled");
					fieldsValid = false;
				}

				const bool* enabled = enabledField != fields->end()
					? enabledField->second.TryGetBoolean()
					: nullptr;
				if (enabledField != fields->end() && !enabled)
				{
					AddError(errors, enabledField->second,
						"system '" + id + "' enabled must be a boolean");
					fieldsValid = false;
				}

				Ludus::Serialization::LSceneValue config =
					Ludus::Serialization::LSceneValue::ObjectValue({}, value.GetLocation());
				if (const auto configField = fields->find("config"); configField != fields->end())
					config = configField->second;

				if (!config.TryGetObject())
				{
					AddError(errors, config, "system '" + id + "' config must be a block");
					fieldsValid = false;
				}

				if (!registry.Contains(id))
				{
					AddError(errors, value, "unknown or unavailable system '" + id + "'");
					fieldsValid = false;
				}
				else if (config.TryGetObject() &&
					!registry.ValidateConfig(id, config, errors))
				{
					fieldsValid = false;
				}

				if (!fieldsValid)
				{
					success = false;
					continue;
				}

				definitions.push_back({ id, *enabled, std::move(config) });
			}
			return success;
		}

		bool LoadEntities(
			const Ludus::Serialization::LSceneValue& root,
			Scene& scene,
			const SceneAssetContext& assets,
			const SceneComponentRegistry& components,
			std::vector<SceneLoadError>& errors)
		{
			const Ludus::Serialization::LSceneValue* entitiesValue = root.Find("entities");
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

				const Ludus::ECS::Entity entity = scene.CreateEntity(id, std::move(displayName));
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
		Ludus::Assets::AssetManager& assets,
		const SceneComponentRegistry& components,
		const SystemRegistry& systems,
		std::vector<SceneLoadError>& errors)
	{
		std::string source;
		if (!Ludus::FileSystem::ReadTextFile(path.c_str(), source))
		{
			errors.push_back({ "failed to read scene file '" + path + "'", { 1, 1 }, path });
			return false;
		}
		return LoadText(source, path, scene, assets, components, systems, errors);
	}

	bool SceneLoader::LoadText(
		std::string_view source,
		const std::string& path,
		Scene& scene,
		Ludus::Assets::AssetManager& assets,
		const SceneComponentRegistry& components,
		const SystemRegistry& systems,
		std::vector<SceneLoadError>& errors)
	{
		const size_t firstError = errors.size();
		const auto finish = [&](bool result)
		{
			for (size_t index = firstError; index < errors.size(); ++index)
				errors[index].path = path;
			return result;
		};

		if (scene.GetWorld().GetEntityCount() != 0 ||
			scene.GetWorld().GetSystemCount() != 0 ||
			!scene.GetSystems().empty())
		{
			errors.push_back({ "scene must be empty before loading", { 1, 1 } });
			return finish(false);
		}

		const Ludus::Serialization::LSceneParseResult parsed = Ludus::Serialization::LSceneParser{}.Parse(source);
		if (!parsed)
		{
			for (const Ludus::Serialization::LSceneParseError& error : parsed.errors)
				errors.push_back({ error.message, error.location });
			return finish(false);
		}

		if (!ValidateRoot(parsed.root, errors))
			return finish(false);

		std::vector<SceneSystemDefinition> systemDefinitions;
		if (!LoadSystemDefinitions(parsed.root, systems, systemDefinitions, errors))
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

		const std::string* sceneName = parsed.root.Find("scene")->TryGetString();
		Ludus::Serialization::LSceneValue assetDeclarations = Ludus::Serialization::LSceneValue::ObjectValue();
		if (const Ludus::Serialization::LSceneValue* value = parsed.root.Find("assets"))
			assetDeclarations = *value;
		stagedScene.SetSerializationData(
			sceneName ? *sceneName : "Untitled",
			std::move(assetDeclarations),
			std::move(assetContext),
			std::move(systemDefinitions));

		scene.Swap(stagedScene);
		for (const SceneSystemDefinition& definition : scene.GetSystems())
		{
			if (definition.enabled)
				systems.Create(definition.id, scene.GetWorld(), definition.config);
		}
		return finish(errors.size() == firstError);
	}
}
