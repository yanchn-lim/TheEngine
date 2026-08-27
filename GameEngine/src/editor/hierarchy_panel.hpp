#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "serialization/lscene_value.hpp"

namespace Ludus
{
	class Scene;
}

namespace Ludus::Editor
{
	class EditorCommandHistory;

	class HierarchyPanel
	{
	public:
		void Draw(Ludus::Scene& scene, EditorCommandHistory& history);
		std::string_view GetSelectedEntityId() const noexcept;

	private:
		void DrawEntityHierarchy(Ludus::Scene& scene);
		void DrawSystems(Ludus::Scene& scene, EditorCommandHistory& history);
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
	};
}
