#include "sandbox_application.hpp"

#include <vector>

#include <imgui.h>
#include <imgui_internal.h>

#include "core/engine.hpp"
#include "debug/debug.hpp"
#include "rotator.hpp"
#include "rotator_system.hpp"
#include "scene/scene_component_registry.hpp"
#include "scene/scene_loader.hpp"
#include "scene/scene_serializer.hpp"
#include "scene/system_registry.hpp"
#include "systems/render_system.hpp"

namespace Tests
{
	namespace
	{
		void DrawEditorDockspace()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			const ImGuiID dockspace = ImGui::GetID("LudusEditorDockspaceV2");
			if (!ImGui::DockBuilderGetNode(dockspace))
			{
				ImGui::DockBuilderAddNode(
					dockspace,
					ImGuiDockNodeFlags_DockSpace |
						ImGuiDockNodeFlags_PassthruCentralNode);
				ImGui::DockBuilderSetNodePos(dockspace, viewport->WorkPos);
				ImGui::DockBuilderSetNodeSize(dockspace, viewport->WorkSize);

				ImGuiID center = dockspace;
				ImGuiID left = 0;
				ImGuiID right = 0;
				ImGui::DockBuilderSplitNode(
					center, ImGuiDir_Left, 0.22f, &left, &center);
				ImGui::DockBuilderSplitNode(
					center, ImGuiDir_Right, 0.32f, &right, &center);
				ImGui::DockBuilderDockWindow("Hierarchy", left);
				ImGui::DockBuilderDockWindow("Inspector", right);
				ImGui::DockBuilderDockWindow("Scene", center);
				ImGui::DockBuilderFinish(dockspace);
			}

			ImGui::DockSpaceOverViewport(
				dockspace,
				viewport,
				ImGuiDockNodeFlags_PassthruCentralNode);
		}
	}

    bool SandboxApplication::OnInitialize(Ludus::Engine& engine)
    {
		Ludus::RegisterBuiltInSceneComponents(_components);
		RegisterRotatorSceneComponent(_components);
		_systems.Register<RotatorSystem>();
		_editorActions.Bind(
			EditorAction::Undo,
			{ Ludus::Key::Z, Ludus::Modifier::Control });
		_editorActions.Bind(
			EditorAction::Redo,
			{ Ludus::Key::Y, Ludus::Modifier::Control });
		_editorActions.Bind(
			EditorAction::Redo,
			{ Ludus::Key::Z,
				Ludus::Modifier::Control | Ludus::Modifier::Shift });
		_editorActions.Bind(
			EditorAction::Save,
			{ Ludus::Key::S, Ludus::Modifier::Control });
		_editorActions.Bind(
			EditorAction::ToggleProfilerPause, { Ludus::Key::F5 });
		_editorActions.Bind(
			EditorAction::PrintProfilerStatistics, { Ludus::Key::F6 });
		_editorActions.Bind(EditorAction::Exit, { Ludus::Key::Escape });
		_actionConnections.push_back(_editorActions.OnPressed(
			EditorAction::Undo,
			[this]
			{
				_history.Undo(_scene, _components);
			}));
		_actionConnections.push_back(_editorActions.OnPressed(
			EditorAction::Redo,
			[this]
			{
				_history.Redo(_scene, _components);
			}));
		_actionConnections.push_back(_editorActions.OnPressed(
			EditorAction::Save,
			[this]
			{
				SaveScene();
			}));
		_actionConnections.push_back(_editorActions.OnPressed(
			EditorAction::ToggleProfilerPause,
			[this]
			{
				_debugOverlay.ToggleProfilerPause();
			}));
		_actionConnections.push_back(_editorActions.OnPressed(
			EditorAction::PrintProfilerStatistics,
			[this]
			{
				_debugOverlay.PrintProfilerStatistics();
			}));
		_actionConnections.push_back(_editorActions.OnPressed(
			EditorAction::Exit,
			[&engine]
			{
				engine.RequestStop();
			}));

        if (!LoadScene(engine, "assets/scenes/maxwell.lscene"))
            return false;

		return true;
    }

	bool SandboxApplication::LoadScene(Ludus::Engine& engine, const char* path)
	{
		std::vector<Ludus::SceneLoadError> errors;
		if (Ludus::SceneLoader::Load(
			path, _scene, engine.GetAssets(), _components, _systems, errors))
		{
			_history.Clear();
			_scenePath = path;
			_scene.GetWorld().AddSystem<Ludus::Systems::RenderSystem>(
				engine.GetRenderEngine());
			return true;
		}

		for (const Ludus::SceneLoadError& error : errors)
		{
			Ludus::Debug::LogError(
				error.path, ":",
				error.location.line, ":",
				error.location.column, ": ",
				error.message);
		}
		return false;
	}

	void SandboxApplication::SaveScene()
	{
		std::vector<std::string> errors;
		if (Ludus::SceneSerializer::Save(
			_scenePath, _scene, _components, _systems, errors))
		{
			_history.MarkSaved();
			_sceneViewportPanel.NotifySaveResult(true);
			return;
		}

		_sceneViewportPanel.NotifySaveResult(false);
		for (const std::string& error : errors)
			Ludus::Debug::LogError("Failed to save scene: ", error);
	}

    void SandboxApplication::OnFixedUpdate(Ludus::Engine&, double fixedDeltaTime)
    {
        _scene.FixedUpdate(fixedDeltaTime);
    }

    void SandboxApplication::OnUpdate(Ludus::Engine&)
    {
        _scene.Update();
    }

    void SandboxApplication::OnImGui(Ludus::Engine& engine)
    {
		_editorActions.Update(
			engine.GetInput(), ImGui::GetIO().WantTextInput, false);
		DrawEditorDockspace();
		_debugOverlay.Draw();
		_sceneViewportPanel.Draw(
			engine.GetRenderEngine(), _history.IsDirty());
		_hierarchyPanel.Draw(_scene, _components, _systems, _history);
		_inspectorPanel.Draw(
			_scene, _components, _history,
			_hierarchyPanel.GetSelectedEntityId());
    }
}
