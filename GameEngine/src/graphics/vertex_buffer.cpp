#include <glad/glad.h>

#include "debug/debug.hpp"
#include "vertex_buffer.hpp"

namespace Graphics
{
	bool VertexBuffer::Create(const void* data, size_t size)
	{
		Destroy();
		if (!data)
		{
			Debug::LogError("VertexBuffer::Create failed: data is null");
			return false;
		}

		if (size == 0)
		{
			Debug::LogError("VertexBuffer::Create failed: size is zero");
			return false;
		}

		glCreateBuffers(1, &_id);
		glNamedBufferData(_id, size, data, GL_STATIC_DRAW);

		if (!IsValid())
		{
			Debug::LogError("VertexBuffer::Create failed: OpenGL buffer was not created");
			return false;
		}

		return true;
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
