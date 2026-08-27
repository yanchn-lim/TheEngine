#pragma once

#include <string>
#include <variant>
#include <vector>

#include "serialization/lscene_value.hpp"

namespace Ludus
{
	class Scene;
	class SceneComponentRegistry;
}

namespace Ludus::Editor
{
	class EditorCommandHistory
	{
	public:
		void RecordComponentEdit(
			std::string entityId,
			std::string componentId,
			Ludus::Serialization::LSceneValue before,
			Ludus::Serialization::LSceneValue after);
		void RecordSystemEdit(
			std::string systemId,
			bool beforeEnabled,
			Ludus::Serialization::LSceneValue beforeConfig,
			bool afterEnabled,
			Ludus::Serialization::LSceneValue afterConfig);

		bool Undo(
			Ludus::Scene& scene,
			const Ludus::SceneComponentRegistry& components);
		bool Redo(
			Ludus::Scene& scene,
			const Ludus::SceneComponentRegistry& components);
		void Clear() noexcept;

		bool CanUndo() const noexcept;
		bool CanRedo() const noexcept;

	private:
		struct ComponentEdit
		{
			std::string entityId;
			std::string componentId;
			Ludus::Serialization::LSceneValue before;
			Ludus::Serialization::LSceneValue after;
		};

		struct SystemEdit
		{
			std::string systemId;
			bool beforeEnabled;
			Ludus::Serialization::LSceneValue beforeConfig;
			bool afterEnabled;
			Ludus::Serialization::LSceneValue afterConfig;
		};

		using Command = std::variant<ComponentEdit, SystemEdit>;

		static bool Apply(
			const Command& command,
			bool useAfter,
			Ludus::Scene& scene,
			const Ludus::SceneComponentRegistry& components);

		std::vector<Command> _undo;
		std::vector<Command> _redo;
	};
}
