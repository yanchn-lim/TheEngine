#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "core/action_map.hpp"
#include "core/application.hpp"
#include "debug/debug_overlay.hpp"
#include "editor/editor_command_history.hpp"
#include "editor/hierarchy_panel.hpp"
#include "editor/inspector_panel.hpp"
#include "editor/scene_viewport_panel.hpp"
#include "scene/scene.hpp"
#include "scene/scene_component_registry.hpp"
#include "scene/system_registry.hpp"

namespace Tests
{
	enum class EditorAction
	{
		Undo,
		Redo,
		Save,
		ToggleProfilerPause,
		PrintProfilerStatistics,
		Exit,
		Count
	};

    class SandboxApplication final : public Ludus::IApplication
    {
    public:
        bool OnInitialize(Ludus::Engine& engine) override;
        void OnFixedUpdate(Ludus::Engine& engine, double fixedDeltaTime) override;
        void OnUpdate(Ludus::Engine& engine) override;
        void OnImGui(Ludus::Engine& engine) override;
	private:
		bool LoadScene(Ludus::Engine& engine, const char* path);
		void SaveScene();

        DebugOverlay _debugOverlay;
		Ludus::SceneComponentRegistry _components;
		Ludus::SystemRegistry _systems;
		Ludus::Editor::HierarchyPanel _hierarchyPanel;
		Ludus::Editor::InspectorPanel _inspectorPanel;
		Ludus::Editor::SceneViewportPanel _sceneViewportPanel;
		Ludus::Editor::EditorCommandHistory _history;
		Ludus::ActionMap<EditorAction> _editorActions;
		std::vector<Ludus::ActionConnection> _actionConnections;
		Ludus::Scene _scene;
		std::filesystem::path _scenePath;
    };
}
