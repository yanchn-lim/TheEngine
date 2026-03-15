#pragma once

struct GLFWwindow;

struct Window
{
	GLFWwindow* handle{ nullptr };
	int2 size{ 1600,900 };
	const char* title{ "Engine" };
	bool vsync = true;

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
private:
	Engine() = default;

	bool Initialize();
	void Update();
	void Shutdown();
};