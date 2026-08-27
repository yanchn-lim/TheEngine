#pragma once

namespace Ludus::Rendering
{
	class RenderEngine;
}

namespace Ludus::Editor
{
	class SceneViewportPanel
	{
	public:
		void Draw(
			Ludus::Rendering::RenderEngine& renderEngine,
			bool sceneDirty);
		void NotifySaveResult(bool succeeded);
		bool IsHovered() const noexcept;
		bool IsFocused() const noexcept;

	private:
		bool _hovered = false;
		bool _focused = false;
		bool _saveSucceeded = false;
		double _saveMessageUntil = 0.0;
	};
}
