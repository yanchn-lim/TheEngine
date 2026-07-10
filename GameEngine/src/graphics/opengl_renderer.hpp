#pragma once

#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

#include "drawcmd.hpp"
#include "camera.hpp"

namespace Graphics
{
	class OpenGLRenderer
	{
	private:
		std::vector<DrawCmd> _cmds;

		//simple camera state
		Camera2D _camera;

	public:
		bool Init();
		void Submit(const DrawCmd&);
		void BeginFrame();
		void EndFrame();
		void Shutdown();


		void SetCamera(const Camera2D& camera);

		//draw helper

	};
}
