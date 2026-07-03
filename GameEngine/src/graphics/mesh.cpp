#include <glad/glad.h>

#include "mesh.hpp"

namespace Graphics
{
	bool Mesh::Create(const float* vertices, uint32_t vertexCount, uint32_t floatsPerVertex)
	{
		Destroy();

		if (!vertices || vertexCount == 0 || floatsPerVertex == 0) return false;

		_vertCnt = vertexCount;

		_vertexArray.Create();
		_vertexBuffer.Create(vertices, vertexCount * floatsPerVertex * sizeof(float));
		_vertexLayout.Add(0, Graphics::ShaderDataType::FLOAT3);
		_vertexLayout.Add(1, Graphics::ShaderDataType::FLOAT3);
		_vertexLayout.Add(2, Graphics::ShaderDataType::FLOAT2);

		_vertexArray.SetVertexBuffer(_vertexBuffer, _vertexLayout);

		return true;
	}

	void Mesh::Bind() const 
	{
		//glBindVertexArray(_vao);
		glBindVertexArray(_vertexArray.GetId());
	}

	void Mesh::Draw() const 
	{
		if (!IsValid())
		{
			return;
		}

		Bind();
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

		_vertCnt = 0;
	}

	uint32_t Mesh::GetVertexCount() const
	{
		return _vertCnt;
	}

	bool Mesh::IsValid() const
	{
		return _vertexArray.IsValid() && _vertCnt != 0 && _vertexBuffer.IsValid();
	}

	Mesh::~Mesh()
	{
		Destroy();
	}

	Mesh::Mesh(Mesh&& oth) noexcept : _vertexArray(std::move(oth._vertexArray)), _vertexBuffer(std::move(oth._vertexBuffer)), _vertCnt(oth._vertCnt)
	{
		oth._vertCnt = 0;
	}

	Mesh& Mesh::operator=(Mesh&& oth) noexcept
	{
		if (&oth == this) return *this;

		Destroy();

		_vertexArray = std::move(oth._vertexArray);
		_vertexBuffer = std::move(oth._vertexBuffer);
		_vertCnt = oth._vertCnt;

		oth._vertCnt = 0;
		oth._vertCnt = 0;

		return *this;
	}
}