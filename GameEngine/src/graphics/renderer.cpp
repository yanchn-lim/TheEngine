#include <glad/glad.h>

#include "debug/profiler.hpp"
#include "drawcmd.hpp"
#include "shader.hpp"
#include "renderer.hpp"
#include "debug/debug.hpp"


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

		constexpr const char* vertexSource = R"(
			#version 460 core

			layout(location = 0) in vec3 aPosition;
			layout(location = 1) in vec3 aColor;
			layout(location = 2) in vec2 aTexCoord;

			uniform mat4 uModel;
			uniform mat4 uView;
			uniform mat4 uProjection;
			
			out vec3 vColor;
			out vec2 vTexcoord;

			void main()
			{
				vColor = aColor;
				gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
				vTexcoord = aTexcoord;		
			}
		)";

		constexpr const char* fragmentSource = R"(
			#version 460 core

			in vec3 vColor;
			in vec2 vTexcoord;
			out vec4 FragColor;

			void main()
			{
				FragColor = vec4(vColor, 1.0);
			}
		)";

		if (!_testShader.Create(vertexSource, fragmentSource))
			return false;
		
		if (!_testShader.IsValid()) return false;

		if(!_testMesh.Create(vertices,6, 8)) return false;

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
			&_testShader
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
