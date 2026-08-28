#include "hierarchy_panel.hpp"

#include <algorithm>
#include <string_view>
#include <vector>

#include <imgui.h>

#include "editor_command_history.hpp"
#include "scene/scene.hpp"
#include "scene/scene_component_registry.hpp"
#include "scene/system_registry.hpp"
#include "value_editor.hpp"

namespace Ludus::Editor
{
	void HierarchyPanel::Draw(
		Ludus::Scene& scene,
		const Ludus::SceneComponentRegistry& components,
		const Ludus::SystemRegistry& systems,
		EditorCommandHistory& history)
	{
		if (!ImGui::Begin("Hierarchy"))
		{
			ImGui::End();
			return;
		}

		if (ImGui::BeginTabBar("HierarchyTabs"))
		{
			if (ImGui::BeginTabItem("Entity Hierarchy"))
			{
				DrawEntityHierarchy(scene, components, history);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Systems"))
			{
				DrawSystems(scene, systems, history);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::End();
	}

	std::string_view HierarchyPanel::GetSelectedEntityId() const noexcept
	{
		return _selectedEntityId;
	}

	void HierarchyPanel::SetSelectedEntityId(std::string_view id)
	{
		_selectedEntityId = id;
		_selectedSystemId.clear();
	}

	void HierarchyPanel::DrawEntityHierarchy(
		Ludus::Scene& scene,
		const Ludus::SceneComponentRegistry& components,
		EditorCommandHistory& history)
	{
		if (!_selectedEntityId.empty() &&
			!scene.FindEntity(_selectedEntityId).IsValid())
		{
			_selectedEntityId.clear();
		}

		for (const std::string& error : _errors)
			ImGui::TextColored(
				ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", error.c_str());

		std::vector<std::string_view> ids;
		ids.reserve(scene.GetEntities().size());
		for (const auto& [id, record] : scene.GetEntities())
		{
			if (scene.GetWorld().IsEntityAlive(record.entity))
				ids.push_back(id);
		}
		std::ranges::sort(ids);

		bool entityDeleted = false;
		for (const std::string_view id : ids)
		{
			const auto found = scene.GetEntities().find(std::string(id));
			if (found == scene.GetEntities().end())
				continue;

			const std::string label = found->second.name + "##entity_" + found->first;
			if (ImGui::Selectable(
				label.c_str(), _selectedEntityId == found->first))
			{
				_selectedEntityId = found->first;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", found->first.c_str());
			if (ImGui::BeginPopupContextItem())
			{
				_selectedEntityId = found->first;
				if (ImGui::MenuItem("Delete Entity"))
				{
					_errors.clear();
					if (history.DeleteEntity(
						scene, components, found->first, _errors))
					{
						_selectedEntityId.clear();
						entityDeleted = true;
					}
				}
				ImGui::EndPopup();
			}
			if (entityDeleted)
				break;
		}

		bool createEntity = false;
		if (ImGui::BeginPopupContextWindow(
			"EntityHierarchyContext",
			ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::MenuItem("Create Entity"))
				createEntity = true;
			ImGui::EndPopup();
		}
		if (createEntity)
		{
			_newEntityName[0] = '\0';
			_errors.clear();
			ImGui::OpenPopup("Create Entity");
		}

		if (ImGui::BeginPopupModal("Create Entity", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::InputText("Name", _newEntityName, sizeof(_newEntityName));
			if (ImGui::Button("Create"))
			{
				_errors.clear();
				const std::string name(_newEntityName);
				if (name.empty())
					_errors.push_back("Entity name must not be empty");
				else if (const std::string id = history.CreateEntity(scene, name);
					!id.empty())
				{
					_selectedEntityId = id;
					ImGui::CloseCurrentPopup();
				}
				else
					_errors.push_back("Failed to generate a unique entity ID");
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();
			for (const std::string& error : _errors)
				ImGui::TextColored(
					ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", error.c_str());
			ImGui::EndPopup();
		}
	}

	void HierarchyPanel::DrawSystems(
		Ludus::Scene& scene,
		const Ludus::SystemRegistry& registry,
		EditorCommandHistory& history)
	{
		auto& systems = scene.GetSystems();
		const auto selected = std::ranges::find(
			systems, _selectedSystemId, &Ludus::SceneSystemDefinition::id);
		if (!_selectedSystemId.empty() && selected == systems.end())
		{
			_activeSystemEdit.reset();
			_selectedSystemId.clear();
		}

		ImGui::BeginChild("SystemList", ImVec2(0.0f, 160.0f), true);
		bool systemRemoved = false;
		for (const Ludus::SceneSystemDefinition& system : systems)
		{
			const std::string label = system.id + "##system_" + system.id;
			if (ImGui::Selectable(
				label.c_str(), _selectedSystemId == system.id))
			{
				if (_selectedSystemId != system.id)
					FinishSystemEdit(history);
				_selectedSystemId = system.id;
			}
			if (ImGui::BeginPopupContextItem())
			{
				if (_selectedSystemId != system.id)
					FinishSystemEdit(history);
				_selectedSystemId = system.id;
				if (ImGui::MenuItem("Remove System"))
				{
					FinishSystemEdit(history);
					if (history.RemoveSystem(scene, system.id))
					{
						_selectedSystemId.clear();
						systemRemoved = true;
					}
				}
				ImGui::EndPopup();
			}
			if (systemRemoved)
				break;
		}
		if (ImGui::BeginPopupContextWindow(
			"SystemListContext",
			ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::BeginMenu("Add System"))
			{
				for (const std::string& id : registry.GetIds())
				{
					const bool exists = std::ranges::find(
						systems, id, &Ludus::SceneSystemDefinition::id) != systems.end();
					ImGui::BeginDisabled(exists);
					if (ImGui::MenuItem(id.c_str()))
					{
						Ludus::Serialization::LSceneValue config =
							Ludus::Serialization::LSceneValue::ObjectValue();
						std::vector<Ludus::SceneLoadError> errors;
						if (registry.CreateDefaultConfig(id, config, errors) &&
							history.AddSystem(
								scene, { id, true, std::move(config) }))
							_selectedSystemId = id;
					}
					ImGui::EndDisabled();
				}
				ImGui::EndMenu();
			}
			ImGui::EndPopup();
		}
		ImGui::EndChild();

		if (_selectedSystemId.empty())
			return;

		auto found = std::ranges::find(
			systems, _selectedSystemId, &Ludus::SceneSystemDefinition::id);
		if (found == systems.end())
			return;

		ImGui::SeparatorText(found->id.c_str());
		const bool beforeEnabled = found->enabled;
		if (ImGui::Checkbox("Enabled", &found->enabled))
		{
			history.RecordSystemEdit(
				found->id,
				beforeEnabled,
				found->config,
				found->enabled,
				found->config);
		}

		ImGui::SeparatorText("Configuration");
		if (const auto* fields = found->config.TryGetObject())
		{
			if (fields->empty())
			{
				ImGui::TextDisabled("No configuration");
				return;
			}
		}
		const Ludus::Serialization::LSceneValue beforeConfig = found->config;
		const ValueEditResult edit = DrawObjectFields(found->config);
		if (edit.changed)
		{
			if (!_activeSystemEdit)
				_activeSystemEdit = ActiveSystemEdit{
					found->id, found->enabled, beforeConfig, found->config };
			else
				_activeSystemEdit->after = found->config;
		}
		if (edit.finished)
			FinishSystemEdit(history);
	}

	void HierarchyPanel::FinishSystemEdit(EditorCommandHistory& history)
	{
		if (!_activeSystemEdit)
			return;

		history.RecordSystemEdit(
			_activeSystemEdit->id,
			_activeSystemEdit->enabled,
			std::move(_activeSystemEdit->before),
			_activeSystemEdit->enabled,
			std::move(_activeSystemEdit->after));
		_activeSystemEdit.reset();
	}
}
