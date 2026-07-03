#include <glad/glad.h>

#include "debug/debug.hpp"
#include "debug/profiler.hpp"
#include "mesh.hpp"
#include "renderer.hpp"
#include "shader.hpp"
#include "texture2d.hpp"


namespace Graphics
{
	
	bool Renderer::Init()
	{
		PROFILE_FUNCTION();
		//init data container
		
		//reserve vector
		_cmds.reserve(8192);

		return true;
	}

	void Renderer::Submit(const DrawCmd& cmd)
	{
		PROFILE_FUNCTION();
		_cmds.emplace_back(cmd);
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
			if (!cmd.shader)
			{
				Debug::LogError("Renderer::EndFrame skipped draw command: shader is null");
				continue;
			}

			if (!cmd.mesh)
			{
				Debug::LogError("Renderer::EndFrame skipped draw command: mesh is null");
				continue;
			}

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
	}

	void Renderer::SetCamera(const Camera2D& camera)
	{
		_camera = camera;
	}
}
