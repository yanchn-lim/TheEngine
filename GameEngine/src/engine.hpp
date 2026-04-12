#pragma once

#include "profiler_ui.hpp"
#include "camera.hpp"

struct GLFWwindow;

struct Window
{
	GLFWwindow* handle{ nullptr };
	int2 size{ 1600,900 };
	const char* title{ "Engine" };
	bool vsync = false;

	bool Init();
	void Shutdown();
};

struct ImGuiLayer
{
	bool Init(GLFWwindow* window);
	void Begin();   // call at start of frame
	void End();     // call at end of frame, before SwapBuffers
	void Shutdown();
};

// tracks raw input state for camera controls
struct InputState
{
	bool  middleMouseHeld = false;
	float2 lastMousePos = float2(0.f);
};


class Engine
{
public:
	static Engine& Get()
	{
		static Engine engine;
		return engine;
	}

	//set singleton
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	int Run();

	Window window;
	ImGuiLayer imgui;

	ProfilerUI profilerUI;

	Camera2D   camera;
	InputState input;

	bool running = false;
private:
	Engine() = default;

	bool Initialize();
	void Update();
	void Shutdown();
};