#include "value_editor.hpp"

#include <string>

#include <imgui.h>

#include "serialization/lscene_value.hpp"

namespace Ludus::Editor
{
	namespace
	{
		int ResizeString(ImGuiInputTextCallbackData* data)
		{
			auto* value = static_cast<std::string*>(data->UserData);
			value->resize(static_cast<size_t>(data->BufTextLen));
			data->Buf = value->data();
			return 0;
		}

		bool InputString(const char* label, std::string& value)
		{
			return ImGui::InputText(
				label,
				value.data(),
				value.capacity() + 1,
				ImGuiInputTextFlags_CallbackResize,
				ResizeString,
				&value);
		}
	}

	ValueEditResult DrawValue(
		const std::string& label,
		Ludus::Serialization::LSceneValue& value)
	{
		ImGui::PushID(label.c_str());
		ValueEditResult result;

		if (auto* boolean = value.TryGetBoolean())
			result.changed = ImGui::Checkbox(label.c_str(), boolean);
		else if (auto* integer = value.TryGetInteger())
			result.changed = ImGui::DragScalar(
				label.c_str(), ImGuiDataType_S64, integer, 1.0f);
		else if (auto* floating = value.TryGetFloat())
			result.changed = ImGui::DragScalar(
				label.c_str(), ImGuiDataType_Double, floating, 0.1f);
		else if (auto* text = value.TryGetString())
			result.changed = InputString(label.c_str(), *text);
		else if (auto* array = value.TryGetArray())
		{
			if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				for (size_t index = 0; index < array->size(); ++index)
				{
					ImGui::PushID(static_cast<int>(index));
					const ValueEditResult child = DrawValue(
						"[" + std::to_string(index) + "]", (*array)[index]);
					result.changed = result.changed || child.changed;
					result.finished = result.finished || child.finished;
					ImGui::PopID();
				}
				ImGui::TreePop();
			}
		}
		else if (auto* object = value.TryGetObject())
		{
			if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				for (auto& [name, child] : *object)
				{
					const ValueEditResult childResult = DrawValue(name, child);
					result.changed = result.changed || childResult.changed;
					result.finished = result.finished || childResult.finished;
				}
				ImGui::TreePop();
			}
		}
		else
		{
			ImGui::TextDisabled("%s: unsupported value", label.c_str());
		}
		if (!value.TryGetArray() && !value.TryGetObject())
			result.finished = ImGui::IsItemDeactivatedAfterEdit();

		ImGui::PopID();
		return result;
	}

	ValueEditResult DrawObjectFields(Ludus::Serialization::LSceneValue& value)
	{
		auto* fields = value.TryGetObject();
		if (!fields)
			return DrawValue("Value", value);

		ValueEditResult result;
		for (auto& [name, field] : *fields)
		{
			const ValueEditResult fieldResult = DrawValue(name, field);
			result.changed = result.changed || fieldResult.changed;
			result.finished = result.finished || fieldResult.finished;
		}
		return result;
	}
}
