#include "glad/glad.h"
#include "vertex_layout.hpp"

namespace Graphics
{
	VertexAttribFormat GetVertexAttribFormat(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::FLOAT:  return { 1, GL_FLOAT, false, false, sizeof(float) };
		case ShaderDataType::FLOAT2: return { 2, GL_FLOAT, false, false, sizeof(float) };
		case ShaderDataType::FLOAT3: return { 3, GL_FLOAT, false, false, sizeof(float) };
		case ShaderDataType::FLOAT4: return { 4, GL_FLOAT, false, false, sizeof(float) };
		case ShaderDataType::INT:    return { 1, GL_INT,   false, true,  sizeof(int) };
		case ShaderDataType::INT2:   return { 2, GL_INT,   false, true,  sizeof(int) };
		case ShaderDataType::INT3:   return { 3, GL_INT,   false, true,  sizeof(int) };
		case ShaderDataType::INT4:   return { 4, GL_INT,   false, true,  sizeof(int) };
		default:
			assert(false && "Unknown ShaderDataType");
			return {};
		}
	}

	void VertexLayout::Add(uint32_t location, ShaderDataType type)
	{
		attributes.push_back({ location, type, stride });

		const auto fmt = GetVertexAttribFormat(type);
		stride += fmt.componentCount * fmt.componentSize;
	}
}