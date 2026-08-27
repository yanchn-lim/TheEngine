#include "inspector_panel.hpp"

#include <algorithm>

#include <imgui.h>

#include "editor_command_history.hpp"
#include "scene/scene.hpp"
#include "scene/scene_component_registry.hpp"
#include "value_editor.hpp"

namespace Ludus::Editor
{
	void InspectorPanel::Draw(
		Ludus::Scene& scene,
		const Ludus::SceneComponentRegistry& components,
		EditorCommandHistory& history,
		std::string_view selectedEntityId)
	{
		if (_selectedEntityId != selectedEntityId)
		{
			FinishActiveEdits(history);
			_selectedEntityId = selectedEntityId;
			_pendingEdits.clear();
		}

		if (!ImGui::Begin("Inspector"))
		{
			ImGui::End();
			return;
		}

		if (_selectedEntityId.empty())
		{
			ImGui::TextDisabled("No entity selected");
			ImGui::End();
			return;
		}

		const Ludus::ECS::Entity entity = scene.FindEntity(_selectedEntityId);
		if (!entity.IsValid())
		{
			FinishActiveEdits(history);
			_selectedEntityId.clear();
			_pendingEdits.clear();
			ImGui::TextDisabled("No entity selected");
			ImGui::End();
			return;
		}

		const std::string name(scene.GetEntityName(_selectedEntityId));
		ImGui::TextUnformatted(name.c_str());
		ImGui::TextDisabled("ID: %s", _selectedEntityId.c_str());
		ImGui::Separator();

		Ludus::Serialization::LSceneValue::Object values;
		std::vector<std::string> saveErrors;
		if (!components.SaveComponents(
			scene.GetAssetContext(), scene.GetWorld(), entity, values, saveErrors))
		{
			for (const std::string& error : saveErrors)
				ImGui::TextColored(
					ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", error.c_str());
			ImGui::End();
			return;
		}

		std::erase_if(
			_pendingEdits,
			[&values](const auto& pending)
			{
				return !values.contains(pending.first);
			});
		std::erase_if(
			_activeEdits,
			[&values](const auto& active)
			{
				return !values.contains(active.first);
			});

		if (values.empty())
			ImGui::TextDisabled("Entity has no components");

		for (auto& [componentName, currentValue] : values)
		{
			ImGui::PushID(componentName.c_str());
			if (ImGui::CollapsingHeader(
				componentName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto pending = _pendingEdits.find(componentName);
				Ludus::Serialization::LSceneValue editable = pending == _pendingEdits.end()
					? currentValue
					: pending->second.value;

				const ValueEditResult edit = DrawObjectFields(editable);
				if (edit.changed)
				{
					std::vector<Ludus::SceneLoadError> updateErrors;
					if (components.Update(
						componentName,
						editable,
						scene.GetAssetContext(),
						scene.GetWorld(),
						entity,
						updateErrors))
					{
						auto active = _activeEdits.find(componentName);
						if (active == _activeEdits.end())
							_activeEdits.emplace(
								componentName,
								ActiveEdit{ currentValue, editable });
						else
							active->second.after = editable;
						_pendingEdits.erase(componentName);
					}
					else
					{
						std::vector<std::string> messages;
						messages.reserve(updateErrors.size());
						for (Ludus::SceneLoadError& error : updateErrors)
							messages.push_back(std::move(error.message));
						_pendingEdits.insert_or_assign(
							componentName,
							PendingEdit{ std::move(editable), std::move(messages) });
					}
				}
				if (edit.finished)
				{
					auto active = _activeEdits.find(componentName);
					if (active != _activeEdits.end())
					{
						history.RecordComponentEdit(
							_selectedEntityId,
							componentName,
							std::move(active->second.before),
							std::move(active->second.after));
						_activeEdits.erase(active);
					}
				}

				pending = _pendingEdits.find(componentName);
				if (pending != _pendingEdits.end())
				{
					for (const std::string& error : pending->second.errors)
					{
						ImGui::TextColored(
							ImVec4(1.0f, 0.35f, 0.35f, 1.0f),
							"%s",
							error.c_str());
					}
				}
			}
			ImGui::PopID();
		}

		ImGui::End();
	}

	void InspectorPanel::FinishActiveEdits(EditorCommandHistory& history)
	{
		for (auto& [componentName, edit] : _activeEdits)
		{
			history.RecordComponentEdit(
				_selectedEntityId,
				componentName,
				std::move(edit.before),
				std::move(edit.after));
		}
		_activeEdits.clear();
	}
}
