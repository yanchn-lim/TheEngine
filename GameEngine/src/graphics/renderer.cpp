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
		
		// Reserve command storage once; frames clear but keep capacity.
		_cmds.reserve(8192);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		return true;
	}

	void Renderer::Submit(const DrawCmd& cmd)
	{
		// Draw commands are copied into the frame buffer and resolved at EndFrame.
		PROFILE_FUNCTION();
		_cmds.emplace_back(cmd);
	}

	void Renderer::BeginFrame()
	{
		PROFILE_FUNCTION();

		// Start a fresh frame command list and clear the current framebuffer.
		_cmds.clear();

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void Renderer::EndFrame()
	{
		PROFILE_FUNCTION();

		// Resolve render state, resources, and camera uniforms for each command.
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

			switch (state.blendMode)
			{
			case BlendMode::NONE:
				glDisable(GL_BLEND);
				break;
			case BlendMode::ALPHA:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glBlendEquation(GL_FUNC_ADD);
				break;
			case BlendMode::ADDITIVE:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				glBlendEquation(GL_FUNC_ADD);
				break;
			case BlendMode::PREMULTIPLIED_ALPHA:
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				glBlendEquation(GL_FUNC_ADD);
				break;
			case BlendMode::MULTIPLY:
				glEnable(GL_BLEND);
				glBlendFunc(GL_DST_COLOR, GL_ZERO);
				glBlendEquation(GL_FUNC_ADD);
				break;
			default:
				glDisable(GL_BLEND);
				break;
			}

			if (state.depthTest)
				glEnable(GL_DEPTH_TEST);
			else
				glDisable(GL_DEPTH_TEST);

			glDepthMask(state.depthWrite ? GL_TRUE : GL_FALSE);

			if (state.culling)
				glEnable(GL_CULL_FACE);
			else
				glDisable(GL_CULL_FACE);


			shader->Bind();

			// Texture is optional, but sprite/default shaders expect slot 0 when present.
			if (texture)
			{
				texture->Bind(0);
				shader->SetInt("uTexture", 0);
			}

			shader->SetMat4("uModel", cmd.transform);
			shader->SetMat4("uView", _camera.GetView());
			shader->SetMat4("uProjection", _camera.GetProjection());

			cmd.mesh->Draw();
		}
	}

	void Renderer::Shutdown()
	{
		PROFILE_FUNCTION();
	}

	void Renderer::SetCamera(const Camera2D& camera)
	{
		// Camera is copied so callers can build it transiently each frame.
		_camera = camera;
	}
}
