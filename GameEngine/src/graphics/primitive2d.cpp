#include "primitive2d.hpp"

#include <algorithm>
#include <cmath>

namespace Graphics::Primitive2D
{
	namespace
	{
		constexpr float Pi = 3.14159265358979323846f;

		constexpr float TriangleVertices[] =
		{
			// position          // color           // uv
			 0.0f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f, 0.5f, 1.0f,
			 0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
			-0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f
		};

		constexpr float QuadVertices[] =
		{
			// position          // color           // uv
			-0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 1.0f, 0.0f, 1.0f
		};

		constexpr uint32_t QuadIndices[] =
		{
			0, 1, 2,
			2, 3, 0
		};

		void AddVertex(std::vector<float>& vertices, float x, float y)
		{
			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(0.0f);
			vertices.push_back(1.0f);
			vertices.push_back(1.0f);
			vertices.push_back(1.0f);
			vertices.push_back(x + 0.5f);
			vertices.push_back(y + 0.5f);
		}
	}

	Graphics::MeshData Triangle()
	{
		VertexLayout layout;
		layout.Add(0, Graphics::ShaderDataType::FLOAT3);
		layout.Add(1, Graphics::ShaderDataType::FLOAT3);
		layout.Add(2, Graphics::ShaderDataType::FLOAT2);

		return { TriangleVertices, 3, nullptr, 0, layout};
	}

	Graphics::MeshData Quad()
	{
		VertexLayout layout;
		layout.Add(0, Graphics::ShaderDataType::FLOAT3);
		layout.Add(1, Graphics::ShaderDataType::FLOAT3);
		layout.Add(2, Graphics::ShaderDataType::FLOAT2);
		return { QuadVertices, 4, QuadIndices, 6, layout};
	}

	std::vector<float> CreateCircle(uint32_t segments)
	{
		segments = std::max(segments, 3u);

		std::vector<float> vertices;
		vertices.reserve(static_cast<size_t>(segments) * 3u * FloatsPerVertex);

		for (uint32_t i = 0; i < segments; ++i)
		{
			const float a0 = (static_cast<float>(i) / static_cast<float>(segments)) * Pi * 2.0f;
			const float a1 = (static_cast<float>(i + 1) / static_cast<float>(segments)) * Pi * 2.0f;

			AddVertex(vertices, 0.0f, 0.0f);
			AddVertex(vertices, std::cos(a0) * 0.5f, std::sin(a0) * 0.5f);
			AddVertex(vertices, std::cos(a1) * 0.5f, std::sin(a1) * 0.5f);
		}

		return vertices;
	}
}
