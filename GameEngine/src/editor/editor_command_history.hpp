#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "serialization/lscene_value.hpp"

namespace Ludus
{
	class Scene;
	class SceneComponentRegistry;
	struct SceneLoadError;
	struct SceneSystemDefinition;
}

namespace Ludus::Editor
{
	// owns reversible scene edits as serialized values and stable string ids.
	// the Scene remains externally owned and must outlive each operation.
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
		std::string CreateEntity(Ludus::Scene& scene, std::string name);
		bool DeleteEntity(
			Ludus::Scene& scene,
			const Ludus::SceneComponentRegistry& components,
			std::string_view id,
			std::vector<std::string>& errors);
		bool AddComponent(
			Ludus::Scene& scene,
			const Ludus::SceneComponentRegistry& components,
			std::string_view entityId,
			std::string componentId,
			Ludus::Serialization::LSceneValue value,
			std::vector<Ludus::SceneLoadError>& errors);
		bool RemoveComponent(
			Ludus::Scene& scene,
			const Ludus::SceneComponentRegistry& components,
			std::string_view entityId,
			std::string_view componentId,
			std::vector<std::string>& errors);
		bool AddSystem(
			Ludus::Scene& scene,
			Ludus::SceneSystemDefinition definition);
		bool RemoveSystem(Ludus::Scene& scene, std::string_view id);

		bool Undo(
			Ludus::Scene& scene,
			const Ludus::SceneComponentRegistry& components);
		bool Redo(
			Ludus::Scene& scene,
			const Ludus::SceneComponentRegistry& components);
		void Clear() noexcept;

		bool CanUndo() const noexcept;
		bool CanRedo() const noexcept;
		bool IsDirty() const noexcept;
		void MarkSaved() noexcept;

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
		struct EntityPresenceChange
		{
			bool existsAfter;
			std::string id;
			std::string name;
			Ludus::Serialization::LSceneValue::Object components;
		};
		struct ComponentPresenceChange
		{
			bool existsAfter;
			std::string entityId;
			std::string componentId;
			Ludus::Serialization::LSceneValue value;
		};
		struct SystemPresenceChange
		{
			bool existsAfter;
			std::string id;
			bool enabled;
			Ludus::Serialization::LSceneValue config;
			size_t index;
		};

		using Command = std::variant<
			ComponentEdit,
			SystemEdit,
			EntityPresenceChange,
			ComponentPresenceChange,
			SystemPresenceChange>;
		struct RecordedCommand
		{
			Command command;
			uint64_t beforeRevision;
			uint64_t afterRevision;
		};

		void Record(Command command);

		static bool Apply(
			const Command& command,
			bool useAfter,
			Ludus::Scene& scene,
			const Ludus::SceneComponentRegistry& components);

		std::vector<RecordedCommand> _undo;
		std::vector<RecordedCommand> _redo;
		// revisions track the current history branch without using stack size.
		// _savedRevision identifies the state accepted by the last save.
		uint64_t _currentRevision = 0;
		uint64_t _savedRevision = 0;
		uint64_t _nextRevision = 1;
	};
}
