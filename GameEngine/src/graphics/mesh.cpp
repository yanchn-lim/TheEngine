#include <glad/glad.h>

#include "debug/debug.hpp"
#include "mesh.hpp"

namespace Graphics
{
	bool Mesh::Create(const void* vertexData, uint32_t vertexCount, const VertexLayout& layout)
	{
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

	void Mesh::Bind() const 
	{
		_vertexArray.Bind();
	}

	void Mesh::Draw() const 
	{
		if (!IsValid())
		{
			Debug::LogError("Mesh::Draw failed: mesh is invalid");
			return;
		}

		Bind();

		if (_indexBuffer.IsValid())
			glDrawElements(GL_TRIANGLES, _indexBuffer.GetCount(), GL_UNSIGNED_INT, nullptr);
		else
			glDrawArrays(GL_TRIANGLES, 0, _vertCnt);
	}

	void Mesh::Destroy() 
	{
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
		_indexBuffer(std::move(oth._indexBuffer))
	{
		oth._vertCnt = 0;
	}

	Mesh& Mesh::operator=(Mesh&& oth) noexcept
	{
		if (&oth == this) return *this;

		Destroy();

		_vertCnt = oth._vertCnt;
		_vertexArray = std::move(oth._vertexArray);
		_vertexBuffer = std::move(oth._vertexBuffer);
		_vertexLayout = std::move(oth._vertexLayout);
		_indexBuffer = std::move(oth._indexBuffer);

		oth._vertCnt = 0;

		return *this;
	}
}
