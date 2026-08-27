#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "serialization/lscene_value.hpp"

namespace Ludus
{
	class Scene;
	class SceneComponentRegistry;
}

namespace Ludus::Editor
{
	class EditorCommandHistory;

	class InspectorPanel
	{
	public:
		void Draw(
			Ludus::Scene& scene,
			const Ludus::SceneComponentRegistry& components,
			EditorCommandHistory& history,
			std::string_view selectedEntityId);

	private:
		struct PendingEdit
		{
			Ludus::Serialization::LSceneValue value;
			std::vector<std::string> errors;
		};
		struct ActiveEdit
		{
			Ludus::Serialization::LSceneValue before;
			Ludus::Serialization::LSceneValue after;
		};

		void FinishActiveEdits(EditorCommandHistory& history);

		std::string _selectedEntityId;
		std::unordered_map<std::string, PendingEdit> _pendingEdits;
		std::unordered_map<std::string, ActiveEdit> _activeEdits;
	};
}
