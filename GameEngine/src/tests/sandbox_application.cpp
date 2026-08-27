#include "sandbox_application.hpp"

#include <vector>

#include <imgui.h>

#include "core/engine.hpp"
#include "debug/debug.hpp"
#include "rotator.hpp"
#include "rotator_system.hpp"
#include "scene/scene_component_registry.hpp"
#include "scene/scene_loader.hpp"
#include "scene/system_registry.hpp"
#include "systems/render_system.hpp"

namespace Tests
{
    bool SandboxApplication::OnInitialize(Ludus::Engine& engine)
    {
		Ludus::RegisterBuiltInSceneComponents(_components);
		RegisterRotatorSceneComponent(_components);
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
		Ludus::SystemRegistry systems;
		systems.Register<RotatorSystem>();

		std::vector<Ludus::SceneLoadError> errors;
		if (Ludus::SceneLoader::Load(
			path, _scene, engine.GetAssets(), _components, systems, errors))
		{
			_history.Clear();
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
		_debugOverlay.Draw();
		_hierarchyPanel.Draw(_scene, _history);
		_inspectorPanel.Draw(
			_scene, _components, _history,
			_hierarchyPanel.GetSelectedEntityId());
    }
}
