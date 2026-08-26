#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Ludus
{
	class Scene;
	class SceneComponentRegistry;

	class SceneSerializer
	{
	public:
		static bool Serialize(
			const Scene& scene,
			const SceneComponentRegistry& components,
			std::string& output,
			std::vector<std::string>& errors);

		static bool Save(
			const std::filesystem::path& path,
			const Scene& scene,
			const SceneComponentRegistry& components,
			std::vector<std::string>& errors);
	};
}
