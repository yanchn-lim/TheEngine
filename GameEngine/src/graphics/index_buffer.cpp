#include <glad/glad.h>

#include "debug/debug.hpp"
#include "index_buffer.hpp"

namespace Graphics
{
	bool IndexBuffer::Create(const uint32_t* indices, uint32_t count) 
	{
		// Upload index data used by glDrawElements.
		Destroy();
		if (!indices)
		{
			Debug::LogError("IndexBuffer::Create failed: indices are null");
			return false;
		}

		if (count == 0)
		{
			Debug::LogError("IndexBuffer::Create failed: count is zero");
			return false;
		}

		glCreateBuffers(1, &_id);
		glNamedBufferData(_id, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);

		_count = count;
		if (!IsValid())
		{
			Debug::LogError("IndexBuffer::Create failed: OpenGL buffer was not created");
			return false;
		}

		return true;
	}

	void IndexBuffer::Bind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _id);
	}

	void IndexBuffer::Destroy()
	{
		// Reset count with the GL id so validity reflects drawable state.
		if (_id)
		{
			glDeleteBuffers(1, &_id);
			_id = 0;
			_count = 0;
		}
	}

	bool IndexBuffer::IsValid() const
	{
		return _id != 0 && _count != 0;
	}

	uint32_t IndexBuffer::GetCount() const
	{
		return _count;
	}

	uint32_t IndexBuffer::GetId() const
	{
		return _id;
	}

	IndexBuffer::~IndexBuffer()
	{
		Destroy();
	}

	IndexBuffer::IndexBuffer(IndexBuffer&& oth) noexcept : _id(oth._id), _count(oth._count)
	{
		// Transfer index buffer ownership and count metadata.
		oth._id = 0;
		oth._count = 0;
	}

	IndexBuffer& IndexBuffer::operator=(IndexBuffer&& oth) noexcept
	{
		// Replace current GL buffer ownership with another index buffer.
		if (&oth == this) return *this;

		Destroy();

		_id = oth._id;
		_count = oth._count;
		oth._id = 0;
		oth._count = 0;

		return *this;
	}
}
