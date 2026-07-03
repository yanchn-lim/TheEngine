#pragma once

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