#include <glad/glad.h>

#include "mesh.hpp"

namespace Graphics
{
	bool Mesh::Create(const float* vertices, uint32_t vertexCount, uint32_t floatsPerVertex)
	{
		Destroy();

		if (!vertices || vertexCount == 0 || floatsPerVertex == 0) return false;

		_vertCnt = vertexCount;

		//create and assign to id
		glCreateVertexArrays(1, &_vao);
		glCreateBuffers(1, &_vbo);

		const size_t bufferSize = vertexCount * floatsPerVertex * sizeof(float);
		glNamedBufferData(_vbo, bufferSize, vertices, GL_STATIC_DRAW);
		
		const GLsizei stride = static_cast<GLsizei>(floatsPerVertex * sizeof(float));

		glVertexArrayVertexBuffer(_vao, 0, _vbo, 0, stride);

		// position
		glEnableVertexArrayAttrib(_vao, 0);
		glVertexArrayAttribFormat(_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayAttribBinding(_vao, 0, 0);

		// color
		glEnableVertexArrayAttrib(_vao, 1);
		glVertexArrayAttribFormat(_vao, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
		glVertexArrayAttribBinding(_vao, 1, 0);

		//uv
		glEnableVertexArrayAttrib(_vao, 2);
		glVertexArrayAttribFormat(_vao, 2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
		glVertexArrayAttribBinding(_vao, 2, 0);

		return true;
	}

	void Mesh::Bind() const 
	{
		glBindVertexArray(_vao);
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
		if (_vbo)
		{
			glDeleteBuffers(1, &_vbo);
			_vbo = 0;
		}

		if (_vao)
		{
			glDeleteVertexArrays(1, &_vao);
			_vao = 0;
		}

		_vertCnt = 0;
	}

	uint32_t Mesh::GetVertexCount() const
	{
		return _vertCnt;
	}

	bool Mesh::IsValid() const
	{
		return _vao != 0 && _vertCnt != 0 && _vbo != 0;
	}

	Mesh::~Mesh()
	{
		Destroy();
	}

	Mesh::Mesh(Mesh&& oth) noexcept : _vao(oth._vao), _vbo(oth._vbo), _vertCnt(oth._vertCnt)
	{
		oth._vao = 0;
		oth._vbo = 0;
		oth._vertCnt = 0;
	}

	Mesh& Mesh::operator=(Mesh&& oth) noexcept
	{
		if (&oth == this) return *this;

		Destroy();

		_vao = oth._vao;
		_vbo = oth._vbo;
		_vertCnt = oth._vertCnt;

		oth._vao = 0;
		oth._vbo = 0;
		oth._vertCnt = 0;

		return *this;
	}
}