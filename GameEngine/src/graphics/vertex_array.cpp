#include <glad/glad.h>

#include "vertex_array.hpp"

namespace Graphics
{
    bool VertexArray::Create()
    {
        Destroy();

        glCreateVertexArrays(1, &_id);
        return IsValid();
    }

    void VertexArray::Bind() const
    {
        glBindVertexArray(_id);
    }

    void VertexArray::Destroy()
    {
        if (_id)
        {
            glDeleteVertexArrays(1, &_id);
            _id = 0;
        }
    }

    bool VertexArray::IsValid() const
    {
        return _id != 0;
    }

    uint32_t VertexArray::GetId() const
    {
        return _id;
    }

    void VertexArray::SetVertexBuffer(const VertexBuffer& buffer, const VertexLayout& layout)
    {
		glVertexArrayVertexBuffer(_id, 0, buffer.GetId(), 0, layout.stride);

        for (const auto& attrib : layout.attributes)
        {
            const auto format = GetVertexAttribFormat(attrib.type);

            glEnableVertexArrayAttrib(_id, attrib.location);

            if (format.integer)
            {
                glVertexArrayAttribIFormat(
                    _id,
                    attrib.location,
                    format.componentCount,
                    format.glType,
                    attrib.offset
                );
            }
            else
            {
                glVertexArrayAttribFormat(
                    _id,
                    attrib.location,
                    format.componentCount,
                    format.glType,
                    format.normalized ? GL_TRUE : GL_FALSE,
                    attrib.offset
                );
            }

            glVertexArrayAttribBinding(_id, attrib.location, 0);
        }

    }

    void VertexArray::SetIndexBuffer(const IndexBuffer& buffer)
    {
        glVertexArrayElementBuffer(_id, buffer.GetId());
    }

    VertexArray::~VertexArray()
    {
        Destroy();
    }

	VertexArray::VertexArray(VertexArray&& oth) noexcept : _id(oth._id)
    {
		oth._id = 0;
    }

    VertexArray& VertexArray::operator=(VertexArray&& oth) noexcept
    {
        if (this != &oth)
        {
            Destroy();
            _id = oth._id;
            oth._id = 0;
		}

        return *this;
    }
}