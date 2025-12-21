#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>
#include <memory>

#include "Core/application.hpp"
#include "Core/time.hpp"
#include "Core/debug_system.hpp"
#include "Input/input.hpp"
#include "Rendering/render_system.hpp"
#include "events.hpp"

namespace
{
	Engine::EngineCtx* s_ActiveCtx = nullptr;

	void GLFWErrorCallback(int error, const char* desc)
	{
		std::cerr << "GLFW Error (" << error << ") : " << desc << "\n";
	}

	void FramebufferSizeCallback(GLFWwindow* /*window*/, int width, int height)
	{
		glViewport(0, 0, width, height);

		if (s_ActiveCtx)
		{
			s_ActiveCtx->windowWidth = width;
			s_ActiveCtx->windowHeight = height;
		}
	}
}

namespace Engine
{
	Application::Application(const ApplicationConfig& /*config*/)
	{
		//empty for now
	}

	//destroying application obj
	Application::~Application()
	{
		if (m_Ctx.window)
		{
			ShutdownEngine();
		}
	}

	int Application::Run()
	{
		ApplicationConfig config;
		InitializeEngine(config);

		OnInitialize();

		m_IsRunning = true;
		MainLoop();

		OnShutdown();
		ShutdownEngine();

		return 0;
	}

	void Application::InitializeEngine(const ApplicationConfig& config)
	{
		s_ActiveCtx = &m_Ctx;

		//init GLFW
		glfwSetErrorCallback(GLFWErrorCallback);
		if (!glfwInit())
		{
			std::cerr << "Failed to initialize GLFW\n";
			return;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef _DEBUG
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

		m_Ctx.windowWidth = config.windowWidth;
		m_Ctx.windowHeight = config.windowHeight;

		m_Ctx.window = glfwCreateWindow(
			m_Ctx.windowWidth,
			m_Ctx.windowHeight,
			config.windowTitle,
			nullptr,
			nullptr
		);

		if (!m_Ctx.window)
		{
			std::cerr << "Failed to create GLFW window\n";
			glfwTerminate();
			return;
		}

		glfwMakeContextCurrent(m_Ctx.window);
		glfwSetFramebufferSizeCallback(m_Ctx.window, FramebufferSizeCallback);
		glfwSwapInterval(config.vsync ? 1 : 0);

		//init GLAD
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cerr << "Failed to initialize GLAD\n";
			glfwDestroyWindow(m_Ctx.window);
			m_Ctx.window = nullptr;
			glfwTerminate();
			return;
		}
		glViewport(0, 0, m_Ctx.windowWidth, m_Ctx.windowHeight);


		//init engine systems
		double startTime = glfwGetTime();
		TimeSystem::Initialize(m_Ctx.time, startTime, m_Ctx.timeConfig);
		InputSystem::Initialize(m_Ctx.window, &m_Ctx.eventBus);
		m_DebugSystem = std::make_unique<DebugFrameListener>(m_Ctx.eventBus);
		m_RenderSystem = std::make_unique<RenderSystem>(m_Ctx.world, m_Ctx.eventBus);
	}

	void Application::ShutdownEngine()
	{
		m_RenderSystem.reset();
		m_DebugSystem.reset();

		if (m_Ctx.window)
		{
			glfwDestroyWindow(m_Ctx.window);
			m_Ctx.window = nullptr;
		}

		glfwTerminate();
		s_ActiveCtx = nullptr;
	}

	void Application::MainLoop()
	{
		using namespace Engine;

		while (m_IsRunning && m_Ctx.window && !glfwWindowShouldClose(m_Ctx.window))
		{
			double now = glfwGetTime();
			TimeSystem::BeginFrame(m_Ctx.time, now);

			InputSystem::BeginFrame();
			glfwPollEvents();

			// ===== FRAME START =====
			m_Ctx.eventBus.Emit(FrameStartEvent{ m_Ctx.time });

			// ===== FIXED UPDATE =====
			int stepIndex = 0;
			while (TimeSystem::StepFixed(m_Ctx.time))
			{
				FixedUpdateEvent fixedEvent{
					m_Ctx.time,
					m_Ctx.time.fixedDeltaTime,
					stepIndex++
				};
				m_Ctx.eventBus.Emit(fixedEvent);

				OnFixedUpdate(m_Ctx.time.fixedDeltaTime);
			}
			// ===== END FIXED UPDATE =====
			
			// ===== UPDATE =====
			m_Ctx.eventBus.Emit(UpdateEvent{ m_Ctx.time });
			OnUpdate(m_Ctx.time.deltaTime);
			// ===== END UPDATE =====
		
			// Interpolation alpha
			float alpha = 0.0f;
			if (m_Ctx.time.fixedDeltaTime > 0.0f)
			{
				alpha = std::clamp(
					m_Ctx.time.accumulator / m_Ctx.time.fixedDeltaTime,
					0.0f, 1.0f
				);
			}

			// Clear + render
			glClearColor(0.0f, 0.4f, 0.5f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			// ===== RENDER =====
			m_Ctx.eventBus.Emit(RenderEvent{ m_Ctx.time, alpha });
			OnRender(alpha);

			// ===== END OF FRAME =====
			m_Ctx.eventBus.Emit(FrameEndEvent{ m_Ctx.time });

			glfwSwapBuffers(m_Ctx.window);
		}
	}
}