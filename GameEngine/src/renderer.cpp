#include "profiler.hpp"
#include "drawcmd.hpp"
#include "shader.hpp"
#include "renderer.hpp"
#include "debug.hpp"

#include <glad/glad.h>

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
			-0.5f, -0.5f, 0.0f,  1.0f, 0.2f, 0.2f,
			 0.5f, -0.5f, 0.0f,  0.2f, 1.0f, 0.2f,
			 0.0f,  0.5f, 0.0f,  0.2f, 0.4f, 1.0f,
		};

		constexpr const char* vertexSource = R"(
			#version 460 core

			layout(location = 0) in vec3 aPosition;
			layout(location = 1) in vec3 aColor;

			out vec3 vColor;

			void main()
			{
				vColor = aColor;
				gl_Position = vec4(aPosition, 1.0);
			}
		)";

		constexpr const char* fragmentSource = R"(
			#version 460 core

			in vec3 vColor;
			out vec4 FragColor;

			void main()
			{
				FragColor = vec4(vColor, 1.0);
			}
		)";

		if (!_testTriangleShader.Create(vertexSource, fragmentSource))
			return false;
		
		if (!_testTriangleShader.IsValid()) return false;

		glCreateVertexArrays(1, &_testTriangleVao);
		glCreateBuffers(1, &_testTriangleVbo);
		glNamedBufferData(_testTriangleVbo, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glVertexArrayVertexBuffer(_testTriangleVao, 0, _testTriangleVbo, 0, 6 * sizeof(float));
		glEnableVertexArrayAttrib(_testTriangleVao, 0);
		glVertexArrayAttribFormat(_testTriangleVao, 0, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayAttribBinding(_testTriangleVao, 0, 0);

		glEnableVertexArrayAttrib(_testTriangleVao, 1);
		glVertexArrayAttribFormat(_testTriangleVao, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
		glVertexArrayAttribBinding(_testTriangleVao, 1, 0);

		return true;
	}

	void Renderer::Submit(const DrawCmd& cmd)
	{
		PROFILE_FUNCTION();
		_cmds.emplace_back(cmd);
	}

	DrawCmd Renderer::GetTestTriangleCmd() const
	{
		return DrawCmd
		{
			_testTriangleVao,
			&_testTriangleShader,
			3
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
			if (!cmd.shader) continue;

			//bind shader
			cmd.shader->Bind();

			//draw
			glBindVertexArray(cmd.vao);
			glDrawArrays(GL_TRIANGLES, 0, cmd.vertcnt);
		}
	}

	void Renderer::Shutdown()
	{
		PROFILE_FUNCTION();

		_testTriangleShader.Destroy();
		glDeleteBuffers(1, &_testTriangleVbo);
		glDeleteVertexArrays(1, &_testTriangleVao);

		_testTriangleVbo = 0;
		_testTriangleVao = 0;
	}
}
