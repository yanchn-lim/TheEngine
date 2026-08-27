#include "editor_command_history.hpp"

#include <algorithm>
#include <utility>

#include "scene/scene.hpp"
#include "scene/scene_component_registry.hpp"

namespace Ludus::Editor
{
	void EditorCommandHistory::RecordComponentEdit(
		std::string entityId,
		std::string componentId,
		Ludus::Serialization::LSceneValue before,
		Ludus::Serialization::LSceneValue after)
	{
		_undo.emplace_back(ComponentEdit{
			std::move(entityId),
			std::move(componentId),
			std::move(before),
			std::move(after) });
		_redo.clear();
	}

	void EditorCommandHistory::RecordSystemEdit(
		std::string systemId,
		bool beforeEnabled,
		Ludus::Serialization::LSceneValue beforeConfig,
		bool afterEnabled,
		Ludus::Serialization::LSceneValue afterConfig)
	{
		_undo.emplace_back(SystemEdit{
			std::move(systemId),
			beforeEnabled,
			std::move(beforeConfig),
			afterEnabled,
			std::move(afterConfig) });
		_redo.clear();
	}

	bool EditorCommandHistory::Undo(
		Ludus::Scene& scene,
		const Ludus::SceneComponentRegistry& components)
	{
		if (_undo.empty() || !Apply(_undo.back(), false, scene, components))
			return false;

		_redo.push_back(std::move(_undo.back()));
		_undo.pop_back();
		return true;
	}

	bool EditorCommandHistory::Redo(
		Ludus::Scene& scene,
		const Ludus::SceneComponentRegistry& components)
	{
		if (_redo.empty() || !Apply(_redo.back(), true, scene, components))
			return false;

		_undo.push_back(std::move(_redo.back()));
		_redo.pop_back();
		return true;
	}

	void EditorCommandHistory::Clear() noexcept
	{
		_undo.clear();
		_redo.clear();
	}

	bool EditorCommandHistory::CanUndo() const noexcept
	{
		return !_undo.empty();
	}

	bool EditorCommandHistory::CanRedo() const noexcept
	{
		return !_redo.empty();
	}

	bool EditorCommandHistory::Apply(
		const Command& command,
		bool useAfter,
		Ludus::Scene& scene,
		const Ludus::SceneComponentRegistry& components)
	{
		if (const auto* component = std::get_if<ComponentEdit>(&command))
		{
			const Ludus::ECS::Entity entity = scene.FindEntity(component->entityId);
			if (!entity.IsValid())
				return false;

			std::vector<Ludus::SceneLoadError> errors;
			return components.Update(
				component->componentId,
				useAfter ? component->after : component->before,
				scene.GetAssetContext(),
				scene.GetWorld(),
				entity,
				errors);
		}

		const auto& system = std::get<SystemEdit>(command);
		auto found = std::ranges::find(
			scene.GetSystems(), system.systemId, &Ludus::SceneSystemDefinition::id);
		if (found == scene.GetSystems().end())
			return false;

		found->enabled = useAfter ? system.afterEnabled : system.beforeEnabled;
		found->config = useAfter ? system.afterConfig : system.beforeConfig;
		return true;
	}
}
