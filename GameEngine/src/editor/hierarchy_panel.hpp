#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "serialization/lscene_value.hpp"

namespace Ludus
{
	class Scene;
	class SceneComponentRegistry;
	class SystemRegistry;
}

namespace Ludus::Editor
{
	class EditorCommandHistory;

	class HierarchyPanel
	{
	public:
		void Draw(
			Ludus::Scene& scene,
			const Ludus::SceneComponentRegistry& components,
			const Ludus::SystemRegistry& systems,
			EditorCommandHistory& history);
		std::string_view GetSelectedEntityId() const noexcept;

	private:
		void DrawEntityHierarchy(
			Ludus::Scene& scene,
			const Ludus::SceneComponentRegistry& components,
			EditorCommandHistory& history);
		void DrawSystems(
			Ludus::Scene& scene,
			const Ludus::SystemRegistry& systems,
			EditorCommandHistory& history);
		void FinishSystemEdit(EditorCommandHistory& history);

		struct ActiveSystemEdit
		{
			std::string id;
			bool enabled;
			Ludus::Serialization::LSceneValue before;
			Ludus::Serialization::LSceneValue after;
		};

		std::string _selectedEntityId;
		std::string _selectedSystemId;
		std::optional<ActiveSystemEdit> _activeSystemEdit;
		char _newEntityName[128]{};
		std::vector<std::string> _errors;
	};
}
