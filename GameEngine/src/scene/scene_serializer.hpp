#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Ludus
{
	class Scene;
	class SceneComponentRegistry;
	class SystemRegistry;

	// serialization validates the live scene before Save replaces its file
	// through a sibling temporary file.
	class SceneSerializer
	{
	public:
		static bool Serialize(
			const Scene& scene,
			const SceneComponentRegistry& components,
			const SystemRegistry& systems,
			std::string& output,
			std::vector<std::string>& errors);

		static bool Save(
			const std::filesystem::path& path,
			const Scene& scene,
			const SceneComponentRegistry& components,
			const SystemRegistry& systems,
			std::vector<std::string>& errors);
	};
}
