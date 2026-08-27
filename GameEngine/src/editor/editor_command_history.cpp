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
		Record(ComponentEdit{
			std::move(entityId),
			std::move(componentId),
			std::move(before),
			std::move(after) });
	}

	void EditorCommandHistory::RecordSystemEdit(
		std::string systemId,
		bool beforeEnabled,
		Ludus::Serialization::LSceneValue beforeConfig,
		bool afterEnabled,
		Ludus::Serialization::LSceneValue afterConfig)
	{
		Record(SystemEdit{
			std::move(systemId),
			beforeEnabled,
			std::move(beforeConfig),
			afterEnabled,
			std::move(afterConfig) });
	}

	std::string EditorCommandHistory::CreateEntity(
		Ludus::Scene& scene,
		std::string name)
	{
		std::string id = scene.CreateEntity(name);
		if (id.empty())
			return {};
		const std::string result = id;
		Record(EntityPresenceChange{
			true, std::move(id), std::move(name), {} });
		return result;
	}

	bool EditorCommandHistory::DeleteEntity(
		Ludus::Scene& scene,
		const Ludus::SceneComponentRegistry& components,
		std::string_view id,
		std::vector<std::string>& errors)
	{
		const Ludus::ECS::Entity entity = scene.FindEntity(id);
		if (!entity.IsValid())
			return false;

		Ludus::Serialization::LSceneValue::Object values;
		if (!components.SaveComponents(
			scene.GetAssetContext(), scene.GetWorld(), entity, values, errors))
			return false;

		const std::string name(scene.GetEntityName(id));
		if (!scene.RemoveEntity(id))
			return false;
		Record(EntityPresenceChange{
			false, std::string(id), name, std::move(values) });
		return true;
	}

	bool EditorCommandHistory::AddComponent(
		Ludus::Scene& scene,
		const Ludus::SceneComponentRegistry& components,
		std::string_view entityId,
		std::string componentId,
		Ludus::Serialization::LSceneValue value,
		std::vector<Ludus::SceneLoadError>& errors)
	{
		const Ludus::ECS::Entity entity = scene.FindEntity(entityId);
		if (!entity.IsValid() || components.Has(componentId, scene.GetWorld(), entity) ||
			!components.Load(componentId, value, scene.GetAssetContext(),
				scene.GetWorld(), entity, errors))
			return false;

		Record(ComponentPresenceChange{
			true, std::string(entityId), std::move(componentId), std::move(value) });
		return true;
	}

	bool EditorCommandHistory::RemoveComponent(
		Ludus::Scene& scene,
		const Ludus::SceneComponentRegistry& components,
		std::string_view entityId,
		std::string_view componentId,
		std::vector<std::string>& errors)
	{
		const Ludus::ECS::Entity entity = scene.FindEntity(entityId);
		if (!entity.IsValid())
			return false;

		Ludus::Serialization::LSceneValue::Object values;
		if (!components.SaveComponents(
			scene.GetAssetContext(), scene.GetWorld(), entity, values, errors))
			return false;
		const auto value = values.find(std::string(componentId));
		if (value == values.end() ||
			!components.Remove(componentId, scene.GetWorld(), entity))
			return false;

		Record(ComponentPresenceChange{
			false, std::string(entityId), std::string(componentId), value->second });
		return true;
	}

	bool EditorCommandHistory::AddSystem(
		Ludus::Scene& scene,
		Ludus::SceneSystemDefinition definition)
	{
		auto& systems = scene.GetSystems();
		if (std::ranges::find(systems, definition.id,
			&Ludus::SceneSystemDefinition::id) != systems.end())
			return false;
		const size_t index = systems.size();
		systems.push_back(definition);
		Record(SystemPresenceChange{
			true,
			std::move(definition.id),
			definition.enabled,
			std::move(definition.config),
			index });
		return true;
	}

	bool EditorCommandHistory::RemoveSystem(
		Ludus::Scene& scene,
		std::string_view id)
	{
		auto& systems = scene.GetSystems();
		const auto found = std::ranges::find(
			systems, id, &Ludus::SceneSystemDefinition::id);
		if (found == systems.end())
			return false;
		const size_t index = static_cast<size_t>(found - systems.begin());
		SystemPresenceChange command{
			false, found->id, found->enabled, found->config, index };
		systems.erase(found);
		Record(std::move(command));
		return true;
	}

	void EditorCommandHistory::Record(Command command)
	{
		const uint64_t afterRevision = _nextRevision++;
		_undo.push_back(RecordedCommand{
			std::move(command), _currentRevision, afterRevision });
		_currentRevision = afterRevision;
		_redo.clear();
	}

	bool EditorCommandHistory::Undo(
		Ludus::Scene& scene,
		const Ludus::SceneComponentRegistry& components)
	{
		if (_undo.empty() ||
			!Apply(_undo.back().command, false, scene, components))
			return false;

		_currentRevision = _undo.back().beforeRevision;
		_redo.push_back(std::move(_undo.back()));
		_undo.pop_back();
		return true;
	}

	bool EditorCommandHistory::Redo(
		Ludus::Scene& scene,
		const Ludus::SceneComponentRegistry& components)
	{
		if (_redo.empty() ||
			!Apply(_redo.back().command, true, scene, components))
			return false;

		_currentRevision = _redo.back().afterRevision;
		_undo.push_back(std::move(_redo.back()));
		_redo.pop_back();
		return true;
	}

	void EditorCommandHistory::Clear() noexcept
	{
		_undo.clear();
		_redo.clear();
		_currentRevision = 0;
		_savedRevision = 0;
		_nextRevision = 1;
	}

	bool EditorCommandHistory::CanUndo() const noexcept
	{
		return !_undo.empty();
	}

	bool EditorCommandHistory::CanRedo() const noexcept
	{
		return !_redo.empty();
	}

	bool EditorCommandHistory::IsDirty() const noexcept
	{
		return _currentRevision != _savedRevision;
	}

	void EditorCommandHistory::MarkSaved() noexcept
	{
		_savedRevision = _currentRevision;
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

		if (const auto* system = std::get_if<SystemEdit>(&command))
		{
			auto found = std::ranges::find(
				scene.GetSystems(), system->systemId,
				&Ludus::SceneSystemDefinition::id);
			if (found == scene.GetSystems().end())
				return false;

			found->enabled = useAfter ? system->afterEnabled : system->beforeEnabled;
			found->config = useAfter ? system->afterConfig : system->beforeConfig;
			return true;
		}

		if (const auto* entity = std::get_if<EntityPresenceChange>(&command))
		{
			const bool shouldExist = useAfter == entity->existsAfter;
			if (!shouldExist)
				return scene.RemoveEntity(entity->id);

			const Ludus::ECS::Entity restored = scene.RestoreEntity(
				entity->id, entity->name);
			if (!restored.IsValid())
				return false;
			for (const auto& [id, value] : entity->components)
			{
				std::vector<Ludus::SceneLoadError> errors;
				if (!components.Load(id, value, scene.GetAssetContext(),
					scene.GetWorld(), restored, errors))
				{
					scene.RemoveEntity(entity->id);
					return false;
				}
			}
			return true;
		}

		if (const auto* component = std::get_if<ComponentPresenceChange>(&command))
		{
			const Ludus::ECS::Entity entity = scene.FindEntity(component->entityId);
			if (!entity.IsValid())
				return false;
			const bool shouldExist = useAfter == component->existsAfter;
			if (!shouldExist)
				return components.Remove(
					component->componentId, scene.GetWorld(), entity);
			std::vector<Ludus::SceneLoadError> errors;
			return components.Load(
				component->componentId,
				component->value,
				scene.GetAssetContext(),
				scene.GetWorld(),
				entity,
				errors);
		}

		const auto& system = std::get<SystemPresenceChange>(command);
		auto& systems = scene.GetSystems();
		const bool shouldExist = useAfter == system.existsAfter;
		if (!shouldExist)
		{
			const auto found = std::ranges::find(
				systems, system.id, &Ludus::SceneSystemDefinition::id);
			if (found == systems.end())
				return false;
			systems.erase(found);
			return true;
		}

		if (std::ranges::find(systems, system.id,
			&Ludus::SceneSystemDefinition::id) != systems.end())
			return false;
		const size_t index = std::min(system.index, systems.size());
		systems.insert(
			systems.begin() + index,
			Ludus::SceneSystemDefinition{
				system.id, system.enabled, system.config });
		return true;
	}
}
