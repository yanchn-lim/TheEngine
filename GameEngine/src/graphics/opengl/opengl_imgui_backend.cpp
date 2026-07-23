#include "opengl_imgui_backend.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace Graphics
{
    bool OpenGLImGuiBackend::Initialize(GLFWwindow* window, IGraphicsDevice&)
    {
        // install GLFW input callbacks and select the GLSL version used by ImGui
        return ImGui_ImplGlfw_InitForOpenGL(window, true) && ImGui_ImplOpenGL3_Init("#version 460");
    }

    void OpenGLImGuiBackend::BeginFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
    }

    void OpenGLImGuiBackend::Render(ImDrawData* drawData)
    {
        // render editor draw data and preserve the main OpenGL context across viewports
        ImGui_ImplOpenGL3_RenderDrawData(drawData);
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup);
        }
    }

    void OpenGLImGuiBackend::Shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
    }
}
