#pragma once

#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

#include "drawcmd.hpp"
#include "shader.hpp"
#include "texture2d.hpp"
#include "mesh.hpp"
#include "camera.hpp"

namespace Graphics
{
	struct SpriteRenderResources
	{
		const Mesh* quadMesh{ nullptr };
		const Shader* shader{ nullptr };
		const Texture2D* fallbackTexture{ nullptr };
	};

	class Renderer
	{
	private:
		std::vector<DrawCmd> _cmds;

		Mesh _testMesh;
		Shader _testShader;
		Texture2D _testTexture;

		//simple camera state
		Camera2D _camera;

	public:
		bool Init();
		void Submit(const DrawCmd&);
		DrawCmd GetTestMeshCmd() const;
		SpriteRenderResources GetSpriteRenderResources() const;
		void BeginFrame();
		void EndFrame();
		void Shutdown();


		void SetCamera(const Camera2D& camera);

		//draw helper

	};
}
