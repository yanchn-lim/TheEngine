#pragma once

#include <glad/glad.h>
#include <cstdint>
#include <vector>

namespace Graphics
{
	enum class ShaderDataType
	{
		FLOAT,
		FLOAT2,
		FLOAT3,
		FLOAT4,
		INT,
		INT2,
		INT3,
		INT4
	};

	struct VertexAttribute
	{
		uint32_t location{};
		ShaderDataType type{};
		uint32_t offset{};
	};

	struct VertexAttribFormat
	{
		uint32_t componentCount;
		GLenum glType;
		bool normalized;
		bool integer;
		uint32_t componentSize;
	};

	struct VertexLayout
	{
		uint32_t stride{};
		std::vector<VertexAttribute> attributes{};

		void Add(uint32_t location, ShaderDataType type);
	};

	VertexAttribFormat GetVertexAttribFormat(ShaderDataType type);
}