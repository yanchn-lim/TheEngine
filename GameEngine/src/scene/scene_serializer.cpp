#include "scene_serializer.hpp"

#include <atomic>
#include <fstream>
#include <system_error>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "scene.hpp"
#include "scene_component_registry.hpp"
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
		std::string& output,
		std::vector<std::string>& errors)
	{
		output.clear();
		Serialization::LSceneValue::Object entities;
		for (const auto& [id, record] : scene.GetEntities())
		{
			if (!scene.GetWorld().IsEntityAlive(record.entity))
				continue;

			Serialization::LSceneValue::Object componentValues;
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

			Serialization::LSceneValue::Object entity;
			entity.emplace("name",
				Serialization::LSceneValue::String(record.name, {}));
			entity.emplace("components",
				Serialization::LSceneValue::ObjectValue(std::move(componentValues)));
			entities.emplace(id,
				Serialization::LSceneValue::ObjectValue(std::move(entity)));
		}

		Serialization::LSceneValue::Object root;
		root.emplace("scene",
			Serialization::LSceneValue::String(std::string(scene.GetName()), {}));
		root.emplace("version", Serialization::LSceneValue::Integer(1, {}));
		root.emplace("assets", scene.GetAssetDeclarations());
		root.emplace("entities",
			Serialization::LSceneValue::ObjectValue(std::move(entities)));

		std::string error;
		if (!Serialization::LSceneWriter::Write(
			Serialization::LSceneValue::ObjectValue(std::move(root)),
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
		std::vector<std::string>& errors)
	{
		if (path.empty())
		{
			errors.push_back("scene path must not be empty");
			return false;
		}

		std::string text;
		if (!Serialize(scene, components, text, errors))
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
