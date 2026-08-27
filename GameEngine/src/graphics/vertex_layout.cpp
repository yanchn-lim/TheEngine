#include "vertex_layout.hpp"

namespace Ludus::Graphics
{
	uint32_t GetShaderDataTypeSize(ShaderDataType type)
	{
		// byte size advances attribute offsets and the total stride
		switch (type)
		{
		case ShaderDataType::FLOAT:  return sizeof(float);
		case ShaderDataType::FLOAT2: return sizeof(float) * 2;
		case ShaderDataType::FLOAT3: return sizeof(float) * 3;
		case ShaderDataType::FLOAT4: return sizeof(float) * 4;
		case ShaderDataType::INT:    return sizeof(int);
		case ShaderDataType::INT2:   return sizeof(int) * 2;
		case ShaderDataType::INT3:   return sizeof(int) * 3;
		case ShaderDataType::INT4:   return sizeof(int) * 4;
		default: return 0;
		}
	}

	uint32_t GetShaderDataTypeComponentCount(ShaderDataType type)
	{
		// component count is passed to OpenGL attribute format calls
		switch (type)
		{
		case ShaderDataType::FLOAT:  return 1;
		case ShaderDataType::FLOAT2: return 2;
		case ShaderDataType::FLOAT3: return 3;
		case ShaderDataType::FLOAT4: return 4;
		case ShaderDataType::INT:    return 1;
		case ShaderDataType::INT2:   return 2;
		case ShaderDataType::INT3:   return 3;
		case ShaderDataType::INT4:   return 4;
		default: return 0;
		}
	}

	bool IsShaderDataTypeInteger(ShaderDataType type)
	{
		return type == ShaderDataType::INT
			|| type == ShaderDataType::INT2
			|| type == ShaderDataType::INT3
			|| type == ShaderDataType::INT4;
	}


	void VertexLayout::Add(uint32_t location, ShaderDataType type)
	{
		// attributes are tightly packed in the order they are added
		attributes.push_back({ location, type, stride });

		stride += GetShaderDataTypeSize(type);
	}

	void VertexLayout::Destroy()
	{
		attributes.clear();
		stride = 0;
	}

	bool VertexLayout::IsValid() const
	{
		return attributes.size() != 0;
	}
}
