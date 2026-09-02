#include "scene_viewport_panel.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <imgui.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "assets/asset_manager.hpp"
#include "components/renderable.hpp"
#include "components/transform.hpp"
#include "geometry/intersection.hpp"
#include "rendering/render_engine.hpp"
#include "scene/scene.hpp"

namespace Ludus::Editor
{
	namespace
	{
		constexpr float GameAspectRatio = 16.0f / 9.0f;
		constexpr float DefaultDistance = 5.22f;
		constexpr float DefaultYaw = 0.0f;
		constexpr float DefaultPitch = -0.291f;
		constexpr float DefaultOrbitSpeed = 0.005f;
		constexpr float DefaultPanSpeed = 1.0f;
		constexpr float DefaultZoomSpeed = 0.15f;
		constexpr float MinimumPitch = -glm::half_pi<float>() + 0.0174533f;
		constexpr float MaximumPitch = glm::half_pi<float>() - 0.0174533f;

		bool GetWorldBounds(
			const Ludus::Scene& scene,
			const Ludus::Assets::AssetManager& assets,
			std::string_view entityId,
			Ludus::Geometry::Aabb& output)
		{
			// picking, focus, and overlays share the transformed mesh bounds.
			// hidden or incomplete renderables have no selectable bounds.
			const Ludus::ECS::Entity entity = scene.FindEntity(entityId);
			const auto* transform = scene.GetWorld().TryGetComponent<
				Ludus::Components::Transform>(entity);
			const auto* renderable = scene.GetWorld().TryGetComponent<
				Ludus::Components::Renderable>(entity);
			const Ludus::Assets::MeshAsset* mesh =
				renderable ? assets.Get(renderable->mesh) : nullptr;
			if (!transform || !renderable || !renderable->visible || !mesh)
				return false;

			const glm::mat4 model =
				glm::translate(glm::mat4(1.0f), transform->position) *
				glm::mat4_cast(transform->rotation) *
				glm::scale(glm::mat4(1.0f), transform->scale);
			output.minimum = glm::vec3(std::numeric_limits<float>::max());
			output.maximum = glm::vec3(std::numeric_limits<float>::lowest());
			for (int x = 0; x < 2; ++x)
			{
				for (int y = 0; y < 2; ++y)
				{
					for (int z = 0; z < 2; ++z)
					{
						const glm::vec3 corner{
							x ? mesh->boundsMaximum.x : mesh->boundsMinimum.x,
							y ? mesh->boundsMaximum.y : mesh->boundsMinimum.y,
							z ? mesh->boundsMaximum.z : mesh->boundsMinimum.z };
						const glm::vec3 world = glm::vec3(model * glm::vec4(corner, 1.0f));
						output.minimum = glm::min(output.minimum, world);
						output.maximum = glm::max(output.maximum, world);
					}
				}
			}
			return true;
		}
	}

	SceneViewportPanel::SceneViewportPanel()
	{
		UpdateCamera();
	}

	void SceneViewportPanel::Draw(
		Ludus::Rendering::RenderEngine& renderEngine,
		bool sceneDirty,
		const Ludus::Scene& scene,
		const Ludus::Assets::AssetManager& assets,
		std::string_view selectedEntityId)
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
		if (ImGui::Button(
			_camera.projectionMode == Ludus::Graphics::ProjectionMode::Perspective
				? "Perspective"
				: "Orthographic"))
		{
			_camera.projectionMode =
				_camera.projectionMode == Ludus::Graphics::ProjectionMode::Perspective
					? Ludus::Graphics::ProjectionMode::Orthographic
					: Ludus::Graphics::ProjectionMode::Perspective;
		}
		ImGui::SameLine();
		if (ImGui::Button("Camera Settings"))
			ImGui::OpenPopup("Editor Camera Settings");
		DrawCameraSettings();
		ImGui::SameLine();
		ImGui::Checkbox("Grid", &_gridVisible);
		ImGui::SameLine();
		ImGui::TextDisabled("Alt+LMB orbit  MMB pan  Wheel zoom");
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
		const ImVec2 imageMinimum = ImGui::GetCursorScreenPos();

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
			UpdateNavigation(imageSize.y);
			if (_hovered && !ImGui::GetIO().KeyAlt &&
				ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				PickEntity(
					scene,
					assets,
					{ imageMinimum.x, imageMinimum.y },
					{ imageSize.x, imageSize.y });
			}
			if ((_hovered || _focused) && !ImGui::GetIO().WantTextInput &&
				ImGui::IsKeyPressed(ImGuiKey_F))
			{
				FocusSelection(scene, assets, selectedEntityId);
			}
			DrawSelectionBounds(
				scene,
				assets,
				selectedEntityId,
				{ imageMinimum.x, imageMinimum.y },
				{ imageSize.x, imageSize.y });
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

	bool SceneViewportPanel::IsGridVisible() const noexcept
	{
		return _gridVisible;
	}

	const Ludus::Graphics::Camera& SceneViewportPanel::GetCamera() const noexcept
	{
		return _camera;
	}

	std::optional<std::string> SceneViewportPanel::TakeSelectionRequest()
	{
		std::optional<std::string> result = std::move(_selectionRequest);
		_selectionRequest.reset();
		return result;
	}

	void SceneViewportPanel::DrawCameraSettings()
	{
		if (!ImGui::BeginPopup("Editor Camera Settings"))
			return;

		const char* projection =
			_camera.projectionMode == Ludus::Graphics::ProjectionMode::Perspective
				? "Perspective"
				: "Orthographic";
		if (ImGui::BeginCombo("Projection", projection))
		{
			if (ImGui::Selectable(
				"Perspective",
				_camera.projectionMode == Ludus::Graphics::ProjectionMode::Perspective))
			{
				_camera.projectionMode = Ludus::Graphics::ProjectionMode::Perspective;
			}
			if (ImGui::Selectable(
				"Orthographic",
				_camera.projectionMode == Ludus::Graphics::ProjectionMode::Orthographic))
			{
				_camera.projectionMode = Ludus::Graphics::ProjectionMode::Orthographic;
			}
			ImGui::EndCombo();
		}

		ImGui::DragFloat(
			"Field of View", &_camera.verticalFieldOfViewDegrees,
			0.5f, 1.0f, 179.0f, "%.1f deg");
		ImGui::DragFloat(
			"Orthographic Size", &_camera.orthographicSize,
			0.05f, 0.01f, 10000.0f, "%.2f");
		ImGui::DragFloat(
			"Near Plane", &_camera.nearPlane,
			0.005f, 0.001f, std::max(_camera.farPlane - 0.001f, 0.001f), "%.3f");
		ImGui::DragFloat(
			"Far Plane", &_camera.farPlane,
			1.0f, _camera.nearPlane + 0.001f, 100000.0f, "%.1f");
		ImGui::Separator();
		ImGui::DragFloat("Orbit Speed", &_orbitSpeed, 0.0001f, 0.0001f, 0.05f, "%.4f");
		ImGui::DragFloat("Pan Speed", &_panSpeed, 0.01f, 0.01f, 10.0f, "%.2f");
		ImGui::DragFloat("Zoom Speed", &_zoomSpeed, 0.01f, 0.01f, 1.0f, "%.2f");
		ImGui::Separator();
		if (ImGui::Button("Reset View"))
			ResetView();
		ImGui::SameLine();
		if (ImGui::Button("Reset Settings"))
			ResetSettings();

		ImGui::EndPopup();
	}

	void SceneViewportPanel::ResetView()
	{
		_target = {};
		_distance = DefaultDistance;
		_yaw = DefaultYaw;
		_pitch = DefaultPitch;
		UpdateCamera();
	}

	void SceneViewportPanel::ResetSettings()
	{
		_camera.projectionMode = Ludus::Graphics::ProjectionMode::Perspective;
		_camera.verticalFieldOfViewDegrees = 60.0f;
		_camera.orthographicSize = 10.0f;
		_camera.nearPlane = 0.05f;
		_camera.farPlane = 1000.0f;
		_orbitSpeed = DefaultOrbitSpeed;
		_panSpeed = DefaultPanSpeed;
		_zoomSpeed = DefaultZoomSpeed;
	}

	void SceneViewportPanel::UpdateCamera()
	{
		const float horizontal = std::cos(_pitch);
		const glm::vec3 forward{
			horizontal * std::sin(_yaw),
			std::sin(_pitch),
			-horizontal * std::cos(_yaw) };
		_camera.position = _target - forward * _distance;
		_camera.rotation = glm::quatLookAtRH(forward, { 0.0f, 1.0f, 0.0f });
	}

	void SceneViewportPanel::UpdateNavigation(float viewportHeight)
	{
		if (_hovered && ImGui::GetIO().KeyAlt &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			_orbiting = true;
		if (_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
			_panning = true;
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
			_orbiting = false;
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle))
			_panning = false;

		bool changed = false;
		const ImVec2 delta = ImGui::GetIO().MouseDelta;
		if (_orbiting && (delta.x != 0.0f || delta.y != 0.0f))
		{
			_yaw += delta.x * _orbitSpeed;
			_pitch = std::clamp(
				_pitch - delta.y * _orbitSpeed,
				MinimumPitch,
				MaximumPitch);
			changed = true;
		}

		if (_panning && viewportHeight > 0.0f &&
			(delta.x != 0.0f || delta.y != 0.0f))
		{
			const float visibleHeight =
				_camera.projectionMode == Ludus::Graphics::ProjectionMode::Perspective
					? 2.0f * _distance * std::tan(
						glm::radians(_camera.verticalFieldOfViewDegrees) * 0.5f)
					: _camera.orthographicSize;
			const float unitsPerPixel = visibleHeight / viewportHeight;
			const glm::vec3 right = _camera.rotation * glm::vec3(1.0f, 0.0f, 0.0f);
			const glm::vec3 up = _camera.rotation * glm::vec3(0.0f, 1.0f, 0.0f);
			_target += (-right * delta.x + up * delta.y) * unitsPerPixel * _panSpeed;
			changed = true;
		}

		const float wheel = _hovered ? ImGui::GetIO().MouseWheel : 0.0f;
		if (wheel != 0.0f)
		{
			const float factor = std::exp(-wheel * _zoomSpeed);
			if (_camera.projectionMode == Ludus::Graphics::ProjectionMode::Perspective)
				_distance = std::clamp(_distance * factor, 0.05f, 10000.0f);
			else
				_camera.orthographicSize = std::clamp(
					_camera.orthographicSize * factor, 0.01f, 10000.0f);
			changed = true;
		}

		if (changed)
			UpdateCamera();
	}

	void SceneViewportPanel::PickEntity(
		const Ludus::Scene& scene,
		const Ludus::Assets::AssetManager& assets,
		glm::vec2 imageMinimum,
		glm::vec2 imageSize)
	{
		if (imageSize.x <= 0.0f || imageSize.y <= 0.0f)
			return;

		const ImVec2 mouse = ImGui::GetMousePos();
		const float normalizedX =
			2.0f * (mouse.x - imageMinimum.x) / imageSize.x - 1.0f;
		const float normalizedY =
			1.0f - 2.0f * (mouse.y - imageMinimum.y) / imageSize.y;
		const glm::mat4 inverseViewProjection = glm::inverse(
			_camera.GetProjection() * _camera.GetView());
		// Camera uses zero-to-one normalized depth for both render backends.
		glm::vec4 nearPoint = inverseViewProjection *
			glm::vec4(normalizedX, normalizedY, 0.0f, 1.0f);
		glm::vec4 farPoint = inverseViewProjection *
			glm::vec4(normalizedX, normalizedY, 1.0f, 1.0f);
		nearPoint /= nearPoint.w;
		farPoint /= farPoint.w;
		const glm::vec3 origin = glm::vec3(nearPoint);
		const glm::vec3 direction = glm::normalize(glm::vec3(farPoint - nearPoint));
		const Ludus::Geometry::Ray ray{ origin, direction };

		std::optional<Ludus::Geometry::RayHit> nearestHit;
		std::string nearestEntity;
		for (const auto& [id, record] : scene.GetEntities())
		{
			Ludus::Geometry::Aabb bounds;
			if (!GetWorldBounds(scene, assets, id, bounds))
				continue;

			const auto hit = Ludus::Geometry::Intersect(ray, bounds);
			if (hit && (!nearestHit || hit->distance < nearestHit->distance))
			{
				nearestHit = hit;
				nearestEntity = id;
			}
		}
		_selectionRequest = std::move(nearestEntity);
	}

	void SceneViewportPanel::FocusSelection(
		const Ludus::Scene& scene,
		const Ludus::Assets::AssetManager& assets,
		std::string_view selectedEntityId)
	{
		Ludus::Geometry::Aabb bounds;
		if (selectedEntityId.empty() ||
			!GetWorldBounds(scene, assets, selectedEntityId, bounds))
			return;

		_target = (bounds.minimum + bounds.maximum) * 0.5f;
		const glm::vec3 extent = (bounds.maximum - bounds.minimum) * 0.5f;
		if (_camera.projectionMode == Ludus::Graphics::ProjectionMode::Perspective)
		{
			const float radius = std::max(glm::length(extent), 0.01f);
			_distance = std::clamp(
				1.25f * radius /
					std::tan(glm::radians(_camera.verticalFieldOfViewDegrees) * 0.5f),
				0.05f,
				10000.0f);
		}
		else
		{
			const float requiredHalfHeight = std::max(
				extent.y,
				extent.x / std::max(_camera.aspectRatio, 0.01f));
			_camera.orthographicSize = std::clamp(
				requiredHalfHeight * 2.5f, 0.01f, 10000.0f);
		}
		UpdateCamera();
	}

	void SceneViewportPanel::DrawSelectionBounds(
		const Ludus::Scene& scene,
		const Ludus::Assets::AssetManager& assets,
		std::string_view selectedEntityId,
		glm::vec2 imageMinimum,
		glm::vec2 imageSize)
	{
		Ludus::Geometry::Aabb bounds;
		if (selectedEntityId.empty() || imageSize.x <= 0.0f || imageSize.y <= 0.0f ||
			!GetWorldBounds(scene, assets, selectedEntityId, bounds))
		{
			return;
		}

		const glm::mat4 viewProjection =
			_camera.GetProjection() * _camera.GetView();
		ImVec2 points[8];
		for (int index = 0; index < 8; ++index)
		{
			const glm::vec3 corner{
				(index & 1) ? bounds.maximum.x : bounds.minimum.x,
				(index & 2) ? bounds.maximum.y : bounds.minimum.y,
				(index & 4) ? bounds.maximum.z : bounds.minimum.z };
			const glm::vec4 clip = viewProjection * glm::vec4(corner, 1.0f);
			if (clip.w <= 0.0f)
				return;
			const glm::vec3 normalized = glm::vec3(clip) / clip.w;
			points[index] = {
				imageMinimum.x + (normalized.x * 0.5f + 0.5f) * imageSize.x,
				imageMinimum.y + (0.5f - normalized.y * 0.5f) * imageSize.y };
		}

		constexpr int Edges[][2] = {
			{ 0, 1 }, { 0, 2 }, { 0, 4 },
			{ 1, 3 }, { 1, 5 },
			{ 2, 3 }, { 2, 6 },
			{ 3, 7 },
			{ 4, 5 }, { 4, 6 },
			{ 5, 7 },
			{ 6, 7 }
		};
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImU32 color = IM_COL32(255, 165, 64, 255);
		for (const auto& edge : Edges)
			drawList->AddLine(points[edge[0]], points[edge[1]], color, 2.0f);
	}
}
