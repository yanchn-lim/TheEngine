#pragma once

#include "math.hpp"

namespace Engine
{
	namespace ECS
	{
		struct Transform
		{
			float3 position{ 0.0f,0.0f,0.0f };

			float rotationZ = 0.0f; //replace with quaternion later on
			float3 scale{ 1.0f,1.0f,1.0f };
		};

		struct Renderable
		{
			unsigned int vao = 0;
			unsigned int vertexCount = 0;

			unsigned int shaderProgram = 0;
			int colorUniformLocation = -1;
			int modelUniformLocation = -1;

			float3 color{ 1.0f,1.0f,1.0f };
		};

		struct Sprite2D
		{
			float2 size{ 1.0f,1.0f };
			float4 color{ 1.0f,1.0f,1.0f,1.0f };
		};

		struct Camera2D
		{
			// >1 = zoom in, <1 = zoom out
			float zoom = 1.0f;

			//near/far clip
			float nearClip = -1.0f;
			float farClip = 1.0f;

			//marking as main render camera
			bool primary = true;
		};
	}
}