#include <glad/glad.h>

#include "debug/debug.hpp"
#include "debug/profiler.hpp"
#include "mesh.hpp"
#include "renderer.hpp"
#include "shader.hpp"
#include "texture2d.hpp"
#include "material.hpp"

namespace Graphics
{
	
	bool Renderer::Init()
	{
		PROFILE_FUNCTION();
		//init data container
		
		//reserve vector
		_cmds.reserve(8192);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
			if (!cmd.material)
			{
				Debug::LogError("Renderer::EndFrame skipped draw command: material is null");
				continue;
			}

			const Shader* shader = cmd.material->shader;
			const Texture2D* texture = cmd.material->texture;

			if (!shader)
			{
				Debug::LogError("Renderer::EndFrame skipped draw command: shader is null");
				continue;
			}

			if (!cmd.mesh)
			{
				Debug::LogError("Renderer::EndFrame skipped draw command: mesh is null");
				continue;
			}

			const auto& state = cmd.material->state;

			if (state.blending)
				glEnable(GL_BLEND);
			else
				glDisable(GL_BLEND);

			if (state.depthTest)
				glEnable(GL_DEPTH_TEST);
			else
				glDisable(GL_DEPTH_TEST);

			glDepthMask(state.depthWrite ? GL_TRUE : GL_FALSE);

			if (state.culling)
				glEnable(GL_CULL_FACE);
			else
				glDisable(GL_CULL_FACE);


			//bind shader
			shader->Bind();

			//bind texture
			if (texture)
			{
				texture->Bind(0);
				shader->SetInt("uTexture", 0);
			}

			shader->SetMat4("uModel", cmd.transform);
			shader->SetMat4("uView", _camera.GetView());
			shader->SetMat4("uProjection", _camera.GetProjection());

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
