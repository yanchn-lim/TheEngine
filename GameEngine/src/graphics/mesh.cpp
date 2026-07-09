#include <glad/glad.h>

#include "debug/debug.hpp"
#include "mesh.hpp"

namespace Graphics
{
	static GLenum ToOpenGLTopology(PrimitiveTopology topology)
	{
		// Translate engine draw modes to the OpenGL primitive mode used at draw time.
		switch (topology)
		{
		case PrimitiveTopology::TRIANGLES: return GL_TRIANGLES;
		case PrimitiveTopology::LINES: return GL_LINES;
		case PrimitiveTopology::POINTS: return GL_POINTS;
		default: return GL_TRIANGLES;
		}
	}
}

namespace Graphics
{
	bool Mesh::Create(const void* vertexData, uint32_t vertexCount, const VertexLayout& layout)
	{
		// Upload non-indexed vertex data and configure a VAO for the supplied layout.
		Destroy();

		if (!vertexData)
		{
			Debug::LogError("Mesh::Create failed: vertexData is null");
			return false;
		}

		if (vertexCount == 0)
		{
			Debug::LogError("Mesh::Create failed: vertexCount is zero");
			return false;
		}

		if (!layout.IsValid())
		{
			Debug::LogError("Mesh::Create failed: vertex layout is invalid");
			return false;
		}


		if (!_vertexArray.Create())
		{
			Debug::LogError("Mesh::Create failed: vertex array creation failed");
			return false;
		}

		if (!_vertexBuffer.Create(vertexData, vertexCount * layout.stride))
		{
			Debug::LogError("Mesh::Create failed: vertex buffer creation failed");
			Destroy();
			return false;
		}

		_vertCnt = vertexCount;
		_vertexLayout = layout;
		_vertexArray.SetVertexBuffer(_vertexBuffer, _vertexLayout);

		return true;
	}

	bool Mesh::Create(const void* vertexData, uint32_t vertexCount, const VertexLayout& layout, const uint32_t* indices, uint32_t indexCount)
	{
		// Upload indexed vertex data and attach the index buffer to the VAO.
		Destroy();

		if (!vertexData)
		{
			Debug::LogError("Mesh::Create failed: vertexData is null");
			return false;
		}

		if (vertexCount == 0)
		{
			Debug::LogError("Mesh::Create failed: vertexCount is zero");
			return false;
		}

		if (!indices)
		{
			Debug::LogError("Mesh::Create failed : indices is null");
			return false;
		}

		if (indexCount == 0)
		{
			Debug::LogError("Mesh::Create failed: indexCount is zero");
			return false;
		}

		if (!layout.IsValid())
		{
			Debug::LogError("Mesh::Create failed: vertex layout is invalid");
			return false;
		}


		if (!_vertexArray.Create())
		{
			Debug::LogError("Mesh::Create failed: vertex array creation failed");
			return false;
		}

		if (!_vertexBuffer.Create(vertexData, vertexCount * layout.stride))
		{
			Debug::LogError("Mesh::Create failed: vertex buffer creation failed");
			Destroy();
			return false;
		}

		if (!_indexBuffer.Create(indices, indexCount))
		{
			Debug::LogError("Mesh::Create failed : index buffer creation failed");
			Destroy();
			return false;
		}


		_vertCnt = vertexCount;
		_vertexLayout = layout;
		_vertexArray.SetVertexBuffer(_vertexBuffer, _vertexLayout);
		_vertexArray.SetIndexBuffer(_indexBuffer);
		return true;
	}

	bool Mesh::Create(const MeshUploadData& data)
	{
		// MeshUploadData is the common upload path for procedural and imported mesh sources.
		const bool created = data.indices && data.indexCount > 0
			? Create(data.vertices, data.vertexCount, data.layout, data.indices, data.indexCount)
			: Create(data.vertices, data.vertexCount, data.layout);

		if (created)
			_topology = data.topology;

		return created;
	}

	void Mesh::Bind() const 
	{
		_vertexArray.Bind();
	}

	void Mesh::Draw() const 
	{
		// Draw chooses indexed or non-indexed rendering based on stored buffers.
		if (!IsValid())
		{
			Debug::LogError("Mesh::Draw failed: mesh is invalid");
			return;
		}

		Bind();

		const GLenum mode = ToOpenGLTopology(_topology);

		if (_indexBuffer.IsValid())
			glDrawElements(mode, _indexBuffer.GetCount(), GL_UNSIGNED_INT, nullptr);
		else
			glDrawArrays(mode, 0, _vertCnt);
	}

	void Mesh::Destroy() 
	{
		// Release owned GPU objects; moved-from meshes are already zeroed.
		if (_vertexBuffer.IsValid())
		{
			_vertexBuffer.Destroy();
		}

		if (_vertexArray.IsValid())
		{
			_vertexArray.Destroy();
		}

		if (_indexBuffer.IsValid())
			_indexBuffer.Destroy();

		_vertCnt = 0;
		
		_vertexLayout.Destroy();
	}

	uint32_t Mesh::GetVertexCount() const
	{
		return _vertCnt;
	}

	bool Mesh::IsValid() const
	{
		return _vertexArray.IsValid()
			&& _vertexBuffer.IsValid()
			&& _vertexLayout.IsValid()
			&& _vertCnt != 0;
	}

	Mesh::~Mesh()
	{
		Destroy();
	}

	Mesh::Mesh(Mesh&& oth) noexcept : _vertCnt(oth._vertCnt), 
		_vertexArray(std::move(oth._vertexArray)),
		_vertexBuffer(std::move(oth._vertexBuffer)),  
		_vertexLayout(std::move(oth._vertexLayout)),
		_indexBuffer(std::move(oth._indexBuffer)),
		_topology(oth._topology)
	{
		// Transfer GL object ownership so the moved-from mesh will not delete it.
		oth._vertCnt = 0;
	}

	Mesh& Mesh::operator=(Mesh&& oth) noexcept
	{
		// Destroy current GL resources before taking ownership from the source mesh.
		if (&oth == this) return *this;

		Destroy();

		_vertCnt = oth._vertCnt;
		_vertexArray = std::move(oth._vertexArray);
		_vertexBuffer = std::move(oth._vertexBuffer);
		_vertexLayout = std::move(oth._vertexLayout);
		_indexBuffer = std::move(oth._indexBuffer);
		_topology = oth._topology;
		oth._vertCnt = 0;

		return *this;
	}
}
