#include "scene_viewport_panel.hpp"

#include <algorithm>
#include <cstdint>

#include <imgui.h>

#include "rendering/render_engine.hpp"

namespace Ludus::Editor
{
	namespace
	{
		constexpr float GameAspectRatio = 16.0f / 9.0f;
	}

	void SceneViewportPanel::Draw(
		Ludus::Rendering::RenderEngine& renderEngine,
		bool sceneDirty)
	{
		const char* title = sceneDirty ? "Scene*###Scene" : "Scene###Scene";
		if (!ImGui::Begin(title, nullptr,
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			_hovered = false;
			_focused = false;
			ImGui::End();
			return;
		}

		_focused = ImGui::IsWindowFocused();
		const ImVec2 available = ImGui::GetContentRegionAvail();
		ImVec2 imageSize{ std::max(available.x, 1.0f), 1.0f };
		imageSize.y = imageSize.x / GameAspectRatio;
		if (imageSize.y > available.y)
		{
			imageSize.y = std::max(available.y, 1.0f);
			imageSize.x = imageSize.y * GameAspectRatio;
		}

		const uint32_t width = static_cast<uint32_t>(imageSize.x);
		const uint32_t height = static_cast<uint32_t>(imageSize.y);
		renderEngine.RequestEditorViewportSize(width, height);
		const ImVec2 start = ImGui::GetCursorPos();
		ImGui::SetCursorPos({
			start.x + std::max((available.x - imageSize.x) * 0.5f, 0.0f),
			start.y + std::max((available.y - imageSize.y) * 0.5f, 0.0f) });

		const ImTextureID texture = renderEngine.GetEditorViewportTexture();
		if (texture != ImTextureID_Invalid)
		{
			const bool flip = renderEngine.EditorViewportNeedsVerticalFlip();
			ImGui::Image(
				texture,
				imageSize,
				flip ? ImVec2(0.0f, 1.0f) : ImVec2(0.0f, 0.0f),
				flip ? ImVec2(1.0f, 0.0f) : ImVec2(1.0f, 1.0f));
			_hovered = ImGui::IsItemHovered();
		}
		else
		{
			ImGui::TextDisabled("Viewport is initializing");
			_hovered = false;
		}

		if (ImGui::GetTime() < _saveMessageUntil)
		{
			const ImVec4 color = _saveSucceeded
				? ImVec4(0.31f, 0.78f, 0.58f, 1.0f)
				: ImVec4(0.95f, 0.35f, 0.31f, 1.0f);
			const ImVec2 windowPosition = ImGui::GetWindowPos();
			const ImVec2 contentMinimum = ImGui::GetWindowContentRegionMin();
			const ImVec2 messagePosition{
				windowPosition.x + contentMinimum.x + 8.0f,
				windowPosition.y + contentMinimum.y + 8.0f };
			ImGui::GetWindowDrawList()->AddText(
				messagePosition,
				ImGui::ColorConvertFloat4ToU32(color),
				_saveSucceeded ? "Scene saved" : "Scene save failed");
		}

		ImGui::End();
	}

	void SceneViewportPanel::NotifySaveResult(bool succeeded)
	{
		_saveSucceeded = succeeded;
		_saveMessageUntil = ImGui::GetTime() + 2.0;
	}

	bool SceneViewportPanel::IsHovered() const noexcept
	{
		return _hovered;
	}

	bool SceneViewportPanel::IsFocused() const noexcept
	{
		return _focused;
	}
}
