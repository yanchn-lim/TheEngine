#pragma once

#include "debug/profiler_ui.hpp"
#include "graphics/renderer.hpp"
#include "assets/asset_manager.hpp"
#include "time.hpp"

struct GLFWwindow;

struct Window
{
	GLFWwindow* handle{ nullptr };
	int width{ 1600 };
	int height{ 900 };
	const char* title{ "Engine" };
	bool vsync = false;

	bool Init();
	void Shutdown();
};

struct ImGuiLayer
{
	bool Init(GLFWwindow* window);
	void Begin();
	void End();
	void Shutdown();
};

class Engine
{
public:
	static Engine& Get()
	{
		static Engine engine;
		return engine;
	}

	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	int Run();

	Time time;
	Window window;
	ImGuiLayer imgui;
	ProfilerUI profilerUI;
	Graphics::Renderer renderer;

	//asset managers
	Assets::AssetManager assets;

	bool running = false;

private:
	Engine() = default;

	bool Initialize();
	void Update();
	void Shutdown();
};
