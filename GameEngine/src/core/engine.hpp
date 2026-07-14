#pragma once

#include "debug/profiler_ui.hpp"
#include "graphics/renderer_backend.hpp"
#include "graphics/irenderer.hpp"
#include "assets/asset_manager.hpp"
#include "time.hpp"

#include <memory>

struct GLFWwindow;

struct Window
{
	GLFWwindow* handle{ nullptr };
	int width{ 1600 };
	int height{ 900 };
	const char* title{ "Engine" };
	bool vsync = false;
	bool resizePending{ false };

	bool Init(Graphics::RendererBackend renderbackend);
	void Shutdown();
};

struct ImGuiLayer
{
	bool Init(GLFWwindow* window);
	void Begin();
	void End();
	void Shutdown();
};

struct ManualRenderTest
{
	Assets::ModelHandle model;
	Assets::MaterialHandle material;
	float rotation = 0.0f;

	bool Initialize(Assets::AssetManager& assets);
	void Submit(Graphics::IRenderer& renderer, const Assets::AssetManager& assets, double deltaTime);
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
	std::unique_ptr<Graphics::IRenderer> renderer;
	ManualRenderTest manualRenderTest;

	//engine settings
	Graphics::RendererBackend renderbackend = Graphics::RendererBackend::OPENGL;


	//asset managers
	Assets::AssetManager assets;

	bool running = false;

private:
	Engine() = default;

	bool Initialize();
	void Update();
	void Shutdown();
};
