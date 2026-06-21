#pragma once

#include "profiler_ui.hpp"

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

	Window window;
	ImGuiLayer imgui;
	ProfilerUI profilerUI;

	bool running = false;

private:
	Engine() = default;

	bool Initialize();
	void Update();
	void Shutdown();
};
