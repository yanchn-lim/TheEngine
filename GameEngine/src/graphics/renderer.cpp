#include <glad/glad.h>

#include "debug/profiler.hpp"
#include "debug/debug.hpp"
#include "core/file_system.hpp"
#include "renderer.hpp"


namespace Graphics
{
	
	bool Renderer::Init()
	{
		PROFILE_FUNCTION();
		//init data container
		
		//reserve vector
		_cmds.reserve(8192);

		constexpr float vertices[] =
		{
			// position          // color
			-0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, //bl
			 0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f, //br
			 0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f, 1.0f, //tr

			 0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f, 1.0f, //tr
			-0.5f,  0.5f, 0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f, //tl
			-0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f
		};

		std::string vertexSource;
		std::string fragmentSource;

		if (!FileSystem::ReadTextFile("assets/shaders/sprite.vert", vertexSource))
		{
			Debug::LogError("Failed to read vertex shader");
			return false;
		}

		if (!FileSystem::ReadTextFile("assets/shaders/sprite.frag", fragmentSource))
		{
			Debug::LogError("Failed to read fragment shader");
			return false;
		}

		if (!_testShader.Create(vertexSource, fragmentSource))
			return false;
		
		if (!_testShader.IsValid()) return false;

		if(!_testMesh.Create(vertices,6, 8)) return false;

		//load texture
		if (!_testTexture.LoadFromFile("assets/textures/steak.png"))
			return false;

		return true;
	}

	void Renderer::Submit(const DrawCmd& cmd)
	{
		PROFILE_FUNCTION();
		_cmds.emplace_back(cmd);
	}

	DrawCmd Renderer::GetTestMeshCmd() const
	{
		return DrawCmd
		{
			&_testMesh,
			&_testShader,
			&_testTexture
		};
	}

	void Renderer::BeginFrame()
	{
		PROFILE_FUNCTION();

		//clear cmds
		_cmds.clear();

		//clear color
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void Renderer::EndFrame()
	{
		PROFILE_FUNCTION();

		//render the image
		for (const DrawCmd& cmd : _cmds)
		{
			if (!cmd.shader || !cmd.mesh) continue;

			//bind shader
			cmd.shader->Bind();

			//bind texture
			if (cmd.texture)
			{
				cmd.texture->Bind(0);
				cmd.shader->SetInt("uTexture", 0);
			}

			cmd.shader->SetMat4("uModel", cmd.transform);
			cmd.shader->SetMat4("uView", _camera.GetView());
			cmd.shader->SetMat4("uProjection", _camera.GetProjection());

			//draw
			cmd.mesh->Draw();
		}
	}

	void Renderer::Shutdown()
	{
		PROFILE_FUNCTION();

		_testShader.Destroy();
		_testMesh.Destroy();
	}

	void Renderer::SetCamera(const Camera2D& camera)
	{
		_camera = camera;
	}
}
