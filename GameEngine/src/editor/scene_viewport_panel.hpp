#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <glm/vec3.hpp>

#include "graphics/camera.hpp"

namespace Ludus::Rendering
{
	class RenderEngine;
}

namespace Ludus
{
	class Scene;
}

namespace Ludus::Assets
{
	class AssetManager;
}

namespace Ludus::Editor
{
	// owns the editor camera and translates viewport input into orbit state.
	// scene and asset data are borrowed only while Draw runs.
	class SceneViewportPanel
	{
	public:
		SceneViewportPanel();
		void Draw(
			Ludus::Rendering::RenderEngine& renderEngine,
			bool sceneDirty,
			const Ludus::Scene& scene,
			const Ludus::Assets::AssetManager& assets,
			std::string_view selectedEntityId);
		void NotifySaveResult(bool succeeded);
		bool IsHovered() const noexcept;
		bool IsFocused() const noexcept;
		bool IsGridVisible() const noexcept;
		const Ludus::Graphics::Camera& GetCamera() const noexcept;
		std::optional<std::string> TakeSelectionRequest();

	private:
		void DrawCameraSettings();
		void ResetView();
		void ResetSettings();
		void UpdateCamera();
		void UpdateNavigation(float viewportHeight);
		void PickEntity(
			const Ludus::Scene& scene,
			const Ludus::Assets::AssetManager& assets,
			glm::vec2 imageMinimum,
			glm::vec2 imageSize);
		void FocusSelection(
			const Ludus::Scene& scene,
			const Ludus::Assets::AssetManager& assets,
			std::string_view selectedEntityId);
		void DrawSelectionBounds(
			const Ludus::Scene& scene,
			const Ludus::Assets::AssetManager& assets,
			std::string_view selectedEntityId,
			glm::vec2 imageMinimum,
			glm::vec2 imageSize);

		Ludus::Graphics::Camera _camera;
		glm::vec3 _target{ 0.0f };
		float _distance = 5.22f;
		float _yaw = 0.0f;
		float _pitch = -0.291f;
		float _orbitSpeed = 0.005f;
		float _panSpeed = 1.0f;
		float _zoomSpeed = 0.15f;
		bool _orbiting = false;
		bool _panning = false;
		// the application consumes this stable scene id once after Draw.
		std::optional<std::string> _selectionRequest;
		bool _hovered = false;
		bool _focused = false;
		bool _gridVisible = true;
		bool _saveSucceeded = false;
		double _saveMessageUntil = 0.0;
	};
}
