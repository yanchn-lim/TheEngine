#include "scene_serializer.hpp"

#include <atomic>
#include <fstream>
#include <system_error>
#include <unordered_set>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "scene.hpp"
#include "scene_component_registry.hpp"
#include "system_registry.hpp"
#include "serialization/lscene_writer.hpp"

namespace Ludus
{
	namespace
	{
		std::filesystem::path TemporaryPath(
			const std::filesystem::path& destination,
			std::vector<std::string>& errors)
		{
			static std::atomic_uint64_t nextSuffix = 1;
			for (size_t attempt = 0; attempt < 100; ++attempt)
			{
				std::filesystem::path temporary = destination;
				temporary += ".tmp." + std::to_string(nextSuffix.fetch_add(1));

				std::error_code error;
				const bool exists = std::filesystem::exists(temporary, error);
				if (error)
				{
					errors.push_back("failed to inspect temporary scene path: " + error.message());
					return {};
				}
				if (!exists)
					return temporary;
			}

			errors.push_back("failed to allocate a temporary scene path");
			return {};
		}

		void RemoveTemporaryFile(const std::filesystem::path& path)
		{
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}

		bool ReplaceSceneFile(
			const std::filesystem::path& source,
			const std::filesystem::path& destination,
			std::vector<std::string>& errors)
		{
#ifdef _WIN32
			if (MoveFileExW(
				source.c_str(),
				destination.c_str(),
				MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
				return true;

			errors.push_back(
				"failed to replace scene file, Windows error " +
				std::to_string(GetLastError()));
			return false;
#else
			std::error_code error;
			std::filesystem::rename(source, destination, error);
			if (!error)
				return true;
			errors.push_back("failed to replace scene file: " + error.message());
			return false;
#endif
		}
	}

	bool SceneSerializer::Serialize(
		const Scene& scene,
		const SceneComponentRegistry& components,
		const SystemRegistry& systems,
		std::string& output,
		std::vector<std::string>& errors)
	{
		output.clear();
		Ludus::Serialization::LSceneValue::Object entities;
		for (const auto& [id, record] : scene.GetEntities())
		{
			if (!scene.GetWorld().IsEntityAlive(record.entity))
				continue;

			Ludus::Serialization::LSceneValue::Object componentValues;
			if (!components.SaveComponents(
				scene.GetAssetContext(),
				scene.GetWorld(),
				record.entity,
				componentValues,
				errors))
			{
				errors.push_back("failed to serialize entity '" + id + "'");
				return false;
			}

			Ludus::Serialization::LSceneValue::Object entity;
			entity.emplace("name",
				Ludus::Serialization::LSceneValue::String(record.name, {}));
			entity.emplace("components",
				Ludus::Serialization::LSceneValue::ObjectValue(std::move(componentValues)));
			entities.emplace(id,
				Ludus::Serialization::LSceneValue::ObjectValue(std::move(entity)));
		}

		Ludus::Serialization::LSceneValue::Object systemValues;
		std::unordered_set<std::string> systemIds;
		for (const SceneSystemDefinition& definition : scene.GetSystems())
		{
			if (!systemIds.emplace(definition.id).second)
			{
				errors.push_back("duplicate system id '" + definition.id + "'");
				return false;
			}
			if (!systems.Contains(definition.id))
			{
				errors.push_back(
					"unknown or unavailable system '" + definition.id + "'");
				return false;
			}

			std::vector<SceneLoadError> configErrors;
			if (!systems.ValidateConfig(
				definition.id, definition.config, configErrors))
			{
				for (SceneLoadError& error : configErrors)
					errors.push_back(std::move(error.message));
				return false;
			}

			Ludus::Serialization::LSceneValue::Object fields;
			fields.emplace("enabled",
				Ludus::Serialization::LSceneValue::Boolean(definition.enabled, {}));
			const auto* config = definition.config.TryGetObject();
			if (config && !config->empty())
				fields.emplace("config", definition.config);
			systemValues.emplace(definition.id,
				Ludus::Serialization::LSceneValue::ObjectValue(std::move(fields)));
		}

		Ludus::Serialization::LSceneValue::Object root;
		root.emplace("scene",
			Ludus::Serialization::LSceneValue::String(std::string(scene.GetName()), {}));
		root.emplace("version", Ludus::Serialization::LSceneValue::Integer(1, {}));
		root.emplace("assets", scene.GetAssetDeclarations());
		if (!systemValues.empty())
			root.emplace("systems",
				Ludus::Serialization::LSceneValue::ObjectValue(std::move(systemValues)));
		root.emplace("entities",
			Ludus::Serialization::LSceneValue::ObjectValue(std::move(entities)));

		std::string error;
		if (!Ludus::Serialization::LSceneWriter::Write(
			Ludus::Serialization::LSceneValue::ObjectValue(std::move(root)),
			output,
			error))
		{
			errors.push_back(std::move(error));
			return false;
		}
		return true;
	}

	bool SceneSerializer::Save(
		const std::filesystem::path& path,
		const Scene& scene,
		const SceneComponentRegistry& components,
		const SystemRegistry& systems,
		std::vector<std::string>& errors)
	{
		if (path.empty())
		{
			errors.push_back("scene path must not be empty");
			return false;
		}

		std::string text;
		if (!Serialize(scene, components, systems, text, errors))
			return false;

		const std::filesystem::path temporary = TemporaryPath(path, errors);
		if (temporary.empty())
			return false;

		std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
		if (!file.is_open())
		{
			errors.push_back("failed to open temporary scene file");
			return false;
		}

		file.write(text.data(), static_cast<std::streamsize>(text.size()));
		file.flush();
		const bool written = file.good();
		file.close();
		if (!written || file.fail())
		{
			RemoveTemporaryFile(temporary);
			errors.push_back("failed to write temporary scene file");
			return false;
		}

		if (!ReplaceSceneFile(temporary, path, errors))
		{
			RemoveTemporaryFile(temporary);
			return false;
		}
		return true;
	}
}
