#include "hierarchy_panel.hpp"

#include <algorithm>
#include <string_view>
#include <vector>

#include <imgui.h>

#include "editor_command_history.hpp"
#include "scene/scene.hpp"
#include "value_editor.hpp"

namespace Ludus::Editor
{
	void HierarchyPanel::Draw(Ludus::Scene& scene, EditorCommandHistory& history)
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
				DrawEntityHierarchy(scene);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Systems"))
			{
				DrawSystems(scene, history);
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

	void HierarchyPanel::DrawEntityHierarchy(Ludus::Scene& scene)
	{
		if (!_selectedEntityId.empty() &&
			!scene.FindEntity(_selectedEntityId).IsValid())
		{
			_selectedEntityId.clear();
		}

		std::vector<std::string_view> ids;
		ids.reserve(scene.GetEntities().size());
		for (const auto& [id, record] : scene.GetEntities())
		{
			if (scene.GetWorld().IsEntityAlive(record.entity))
				ids.push_back(id);
		}
		std::ranges::sort(ids);

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
		}
	}

	void HierarchyPanel::DrawSystems(
		Ludus::Scene& scene,
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
