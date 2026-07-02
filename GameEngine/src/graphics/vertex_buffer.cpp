#include <glad/glad.h>

#include "vertex_buffer.hpp"

namespace Graphics
{
	bool VertexBuffer::Create(const void* data, size_t size)
	{
		Destroy();
		if (!data || size == 0) return false;

		glCreateBuffers(1, &_id);
		glNamedBufferData(_id, size, data, GL_STATIC_DRAW);

		return IsValid();
	}

	void VertexBuffer::Destroy()
	{
		if (_id)
		{
			glDeleteBuffers(1, &_id);
			_id = 0;
		}
	}

	uint32_t VertexBuffer::GetId() const
	{
		return _id;
	}

	bool VertexBuffer::IsValid() const
	{
		return _id != 0;
	}

	VertexBuffer::~VertexBuffer()
	{
		Destroy();
	}

	VertexBuffer::VertexBuffer(VertexBuffer&& oth) noexcept : _id(oth._id)
	{
		oth._id = 0;
	}

	VertexBuffer& VertexBuffer::operator=(VertexBuffer&& oth) noexcept
	{
		if (&oth == this) return *this;

		Destroy();

		_id = oth._id;
		oth._id = 0;

		return *this;
	}
}