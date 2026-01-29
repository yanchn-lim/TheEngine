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
#include "Scene/scene_manager.hpp"
#include "events.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

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
		/*double startTime = glfwGetTime();
		TimeSystem::Initialize(m_Ctx.time, startTime, m_Ctx.timeConfig);
		InputSystem::Initialize(m_Ctx.window, &m_Ctx.eventBus);*/
		m_DebugSystem = std::make_unique<DebugFrameListener>(m_Ctx.eventBus);

		if (!m_Ctx.sceneManager.HasActiveScene())
		{
			m_Ctx.sceneManager.SetActiveScene(std::make_unique<Scene>());
		}

		// Get the world from the active scene and give it to RenderSystem
		Scene& scene = m_Ctx.sceneManager.GetActiveScene();
		m_RenderSystem = std::make_unique<RenderSystem>(scene.GetWorld(), m_Ctx.eventBus);

		InitializeImGui();
	}

	void Application::ShutdownEngine()
	{
		ShutdownImGui();

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

			// ===== IMGUI FRAME =====
			if (m_ImGuiInitialized)
			{
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();
			}

			// ===== RENDER =====
			glClearColor(0.0f, 0.4f, 0.5f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			m_Ctx.eventBus.Emit(RenderEvent{ m_Ctx.time, alpha });
			OnRender(alpha);

			// ===== BUILD IMGUI =====
			if (m_ImGuiInitialized)
			{
				//build gui here
				OnRenderGUI();
			}

			// ===== END OF FRAME =====
			m_Ctx.eventBus.Emit(FrameEndEvent{ m_Ctx.time });

			if (m_ImGuiInitialized)
			{
				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			}

			glfwSwapBuffers(m_Ctx.window);
		}
	}

	void Application::InitializeImGui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		//add other flags here

		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForOpenGL(m_Ctx.window, true);
		ImGui_ImplOpenGL3_Init("#version 450");

		m_ImGuiInitialized = true;
	}

	void Application::ShutdownImGui()
	{
		if (!m_ImGuiInitialized) return;

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		m_ImGuiInitialized = false;
	}
}