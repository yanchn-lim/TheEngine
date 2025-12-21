#pragma once

#include "Core/engine_context.hpp"


namespace Engine
{
	struct ApplicationConfig
	{
		const char* windowTitle = "Engine";
		int windowWidth = 1600;
		int windowHeight = 900;
		bool vsync = true;
	};

	class Application
	{
	public:
		Application(const ApplicationConfig& config);
		virtual ~Application();

		int Run();

		EngineCtx& GetContext() { return m_Ctx; }

	protected:
		// Game / Editor overrides
		virtual void OnInitialize() {}
		virtual void OnShutdown() {}

		virtual void OnFixedUpdate(float fixedDeltaTime) {}
		virtual void OnUpdate(float deltaTime) {}
		virtual void OnRender(float alpha) {}

	private:
		void InitializeEngine(const ApplicationConfig& config);
		void ShutdownEngine();
		void MainLoop();

	private:
		EngineCtx m_Ctx{};
		bool      m_IsRunning = false;

		// Engine-owned systems
		std::unique_ptr<class RenderSystem>       m_RenderSystem;
		std::unique_ptr<class DebugFrameListener> m_DebugSystem;
	};
};
