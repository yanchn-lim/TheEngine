#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Ludus
{
	class Scene;
	class SceneComponentRegistry;
	class SystemRegistry;

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
