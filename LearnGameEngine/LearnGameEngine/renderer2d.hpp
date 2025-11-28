#pragma once

#include "math.hpp"

namespace Engine
{
	class Renderer2D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const float4x4& viewProj);
		static void EndScene();

		static void DrawQuad(
			const float3& position,
			const float2& size,
			const float4& color,
			float rotationZ = 0.0f
		);

		static void DrawCircle(
			const float3& position,
			float radius,
			const float4& color
		);

		static void DrawTriangle(
			const float3& p0,
			const float3& p1,
			const float3& p2,
			const float4& color
		);
	};
}