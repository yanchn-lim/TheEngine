#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "debug/debug.hpp"
#include "graphics/primitive_topology.hpp"
#include "graphics/vertex_layout.hpp"

namespace Assets
{
	enum class VertexSemantic
	{
		POSITION,
		NORMAL,
		TANGENT,
		COLOR,
		TEXCOORD0,
		TEXCOORD1,
	};

	struct VertexStream
	{
		VertexSemantic semantic{};
		Graphics::ShaderDataType type{};
		std::vector<std::byte> data{};
	};

	struct ModelMeshData
	{
		uint32_t vertexCount = 0;
		std::vector<VertexStream> streams;
		std::vector<uint32_t> indices;
		Graphics::PrimitiveTopology topology = Graphics::PrimitiveTopology::TRIANGLES;

		bool HasStream(VertexSemantic semantic) const;
		const VertexStream* FindStream(VertexSemantic semantic) const;
		VertexStream* FindStream(VertexSemantic semantic);

		template<typename T>
		void AddStream(VertexSemantic semantic, Graphics::ShaderDataType type, const std::vector<T>& values)
		{
			if (values.empty())
			{
				Debug::LogError("ModelMeshData::AddStream : Data passed in is empty.");
				return;
			}

			if (sizeof(T) != Graphics::GetShaderDataTypeSize(type))
			{
				Debug::LogError("ModelMeshData::AddStream : Data size does not match type's actual size");
				return;
			}

			if (vertexCount == 0)
				vertexCount = static_cast<uint32_t>(values.size());
			else if (values.size() != vertexCount)
			{
				Debug::LogError("ModelMeshData::AddStream : Data count does not match vertex count");
				return;
			}

			VertexStream stream;
			stream.semantic = semantic;
			stream.type = type;

			const std::byte* begin = reinterpret_cast<const std::byte*>(values.data());
			const std::byte* end = begin + values.size() * sizeof(T);

			stream.data.assign(begin, end);

			streams.push_back(std::move(stream));
		}
	};

	struct ModelData
	{
		std::vector<ModelMeshData> meshes;
	};
}