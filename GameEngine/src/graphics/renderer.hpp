#pragma once

#include <cstdint>
#include <vector>

#include "drawcmd.hpp"
#include "shader.hpp"

namespace Graphics
{
	class Renderer
	{
	private:
		std::vector<DrawCmd> _cmds;
		uint32_t _testTriangleVao = 0;
		uint32_t _testTriangleVbo = 0;
		Shader _testTriangleShader;

	public:
		bool Init();
		void Submit(const DrawCmd&);
		DrawCmd GetTestTriangleCmd() const;
		void BeginFrame();
		void EndFrame();
		void Shutdown();
	};
}
