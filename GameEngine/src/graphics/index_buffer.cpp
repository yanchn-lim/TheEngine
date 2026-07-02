#include <glad/glad.h>

#include "index_buffer.hpp"

namespace Graphics
{
	bool IndexBuffer::Create(const uint32_t* indices, uint32_t count) 
	{
		if (!indices || count == 0) return false;

		glCreateBuffers(1, &_id);
		glNamedBufferData(_id, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);

		_count = count;
		return IsValid();
	}

	void IndexBuffer::Bind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,_id);
	}

	void IndexBuffer::Destroy()
	{
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
		oth._id = 0;
		oth._count = 0;
	}

	IndexBuffer& IndexBuffer::operator=(IndexBuffer&& oth) noexcept
	{
		if (&oth == this) return *this;

		Destroy();

		_id = oth._id;
		_count = oth._count;
		oth._id = 0;
		oth._count = 0;

		return *this;
	}
};
