#include "primitive_mesh2d.hpp"

#include <algorithm>
#include <cmath>

namespace Assets::Primitive2D
{
	namespace
	{
		constexpr float Pi = 3.14159265358979323846f;

		MeshVertex MakeVertex(float x, float y)
		{
			// primitive vertices use the engine's standard position, color, and UV layout
			return
			{
				{ x, y, 0.0f },
				{ 1.0f, 1.0f, 1.0f },
				{ x + 0.5f, y + 0.5f }
			};
		}
	}

	MeshSurface Triangle()
	{
		// unit triangle centered around the origin for quick renderer tests
		MeshSurface data;
		data.vertices =
		{
			{ {  0.0f,  0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.5f, 1.0f } },
			{ {  0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
			{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } }
		};
		data.indices = { 0, 1, 2 };
		data.topology = Graphics::PrimitiveTopology::TRIANGLES;

		return data;
	}

	MeshSurface Quad()
	{
		// indexed quad keeps shared corners explicit for texture and material tests
		MeshSurface data;
		data.vertices =
		{
			{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
			{ {  0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
			{ {  0.5f,  0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
			{ { -0.5f,  0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }
		};
		data.indices = { 0, 1, 2, 2, 3, 0 };
		data.topology = Graphics::PrimitiveTopology::TRIANGLES;

		return data;
	}

	MeshSurface Circle(uint32_t segments)
	{
		// build a triangle fan with one center vertex and a configurable rim
		segments = std::max(segments, 3u);

		MeshSurface data;
		data.vertices.reserve(static_cast<size_t>(segments) + 1u);
		data.indices.reserve(static_cast<size_t>(segments) * 3u);

		data.vertices.push_back(MakeVertex(0.0f, 0.0f));

		for (uint32_t i = 0; i < segments; ++i)
		{
			const float angle = (static_cast<float>(i) / static_cast<float>(segments)) * Pi * 2.0f;
			data.vertices.push_back(MakeVertex(std::cos(angle) * 0.5f, std::sin(angle) * 0.5f));
		}

		for (uint32_t i = 0; i < segments; ++i)
		{
			const uint32_t current = i + 1u;
			const uint32_t next = (i + 1u) % segments + 1u;

			data.indices.push_back(0);
			data.indices.push_back(current);
			data.indices.push_back(next);
		}

		data.topology = Graphics::PrimitiveTopology::TRIANGLES;

		return data;
	}
}
