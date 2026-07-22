#pragma once

#include <cstdint>
#include <vector>

namespace Graphics
{
	// lists shader-visible scalar and vector formats without native API values
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

	// maps one vertex field to a shader location and byte offset
	struct VertexAttribute
	{
		uint32_t location{};
		ShaderDataType type{};
		uint32_t offset{};
	};

	// describes the stride and ordered attributes of one vertex buffer
	struct VertexLayout
	{
		uint32_t stride{};
		std::vector<VertexAttribute> attributes{};

		void Add(uint32_t location, ShaderDataType type);
		void Destroy();

		bool IsValid() const;
	};

	uint32_t GetShaderDataTypeSize(ShaderDataType type);
	uint32_t GetShaderDataTypeComponentCount(ShaderDataType type);
	bool IsShaderDataTypeInteger(ShaderDataType type);
}
