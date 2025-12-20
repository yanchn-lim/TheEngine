#pragma once
#include <cmath>

namespace Engine
{
	struct float2
	{
		float x = 0.0f;
		float y = 0.0f;
	};

	struct float3
	{
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
	};

	struct float4
	{
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float w = 0.0f;
	};

	struct float4x4
	{
		float m[16] =
		{
			1,0,0,0,
			0,1,0,0,
			0,0,1,0,
			0,0,0,1
		};
	};

	inline float4x4 MakeModelMatrix(const float3& position, float rotationZ, const float3& scale)
	{
		const float c = std::cos(rotationZ);
		const float s = std::sin(rotationZ);

		float4x4 result{};

		//column 0
		result.m[0] = scale.x * c;
		result.m[1] = scale.x * s;
		result.m[2] = 0.0f;
		result.m[3] = 0.0f;

		//column 1
		result.m[4] = -scale.y * s;
		result.m[5] = scale.y * c;
		result.m[6] = 0.0f;
		result.m[7] = 0.0f;

		//column 2
		result.m[8] = 0.0f;
		result.m[9] = 0.0f;
		result.m[10] = scale.z;
		result.m[11] = 0.0f;

		//column 3
		result.m[12] = position.x;
		result.m[13] = position.y;
		result.m[14] = position.z;
		result.m[15] = 1.0f;

		return result;
	}

	inline float4x4 MakeOrtho(
		float left, float right,
		float bottom, float top,
		float zNear, float zFar)
	{
		float4x4 r{}; // zero-init

		const float rl = right - left;
		const float tb = top - bottom;
		const float fn = zFar - zNear;

		r.m[0] = 2.0f / rl;
		r.m[5] = 2.0f / tb;
		r.m[10] = -2.0f / fn;
		r.m[12] = -(right + left) / rl;
		r.m[13] = -(top + bottom) / tb;
		r.m[14] = -(zFar + zNear) / fn;
		r.m[15] = 1.0f;

		return r;
	}
}