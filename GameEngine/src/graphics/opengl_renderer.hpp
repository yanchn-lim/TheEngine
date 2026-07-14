#pragma once

#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

#include "irenderer.hpp"

namespace Graphics
{
	class OpenGLRenderer : public IRenderer
	{
	private:
		GLFWwindow* _window{ nullptr };
		std::vector<DrawCmd> _cmds;

		//simple camera state
		Camera2D _camera;

	public:
		bool Init(GLFWwindow*) override;
		void Submit(const DrawCmd&) override;
		void BeginFrame() override;
		void EndFrame() override;
		void Present() override;
		void SetCamera(const Camera2D& camera) override;
		void OnResize(uint32_t width, uint32_t height) override;
		void Shutdown() override;



		//draw helper

	};
}
