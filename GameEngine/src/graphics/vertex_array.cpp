#include <glad/glad.h>

#include "vertex_array.hpp"
#include "debug/debug.hpp"

namespace Graphics
{
    static GLenum ToOpenGLBaseType(ShaderDataType type)
    {
        switch (type)
        {
        case ShaderDataType::FLOAT:
        case ShaderDataType::FLOAT2:
        case ShaderDataType::FLOAT3:
        case ShaderDataType::FLOAT4:
            return GL_FLOAT;

        case ShaderDataType::INT:
        case ShaderDataType::INT2:
        case ShaderDataType::INT3:
        case ShaderDataType::INT4:
            return GL_INT;

        default:
            return GL_FLOAT;
        }
    }
}

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
        if (!buffer.IsValid() || !layout.IsValid())
        {
            Debug::LogError("VertexArray::SetVertexBuffer failed : Buffer or Layout is invalid!\n");
            return;
        }

        if (!IsValid())
        {
            Debug::LogError("VertexArray::SetVertexBuffer failed : VertexArray is invalid!\n");
            return;
        }

		glVertexArrayVertexBuffer(_id, 0, buffer.GetId(), 0, layout.stride);

        for (const auto& attrib : layout.attributes)
        {
            const uint32_t componentCount = GetShaderDataTypeComponentCount(attrib.type);
            const GLenum glType = ToOpenGLBaseType(attrib.type);

            glEnableVertexArrayAttrib(_id, attrib.location);

            if (IsShaderDataTypeInteger(attrib.type))
            {
                glVertexArrayAttribIFormat(
                    _id,
                    attrib.location,
                    componentCount,
                    glType,
                    attrib.offset
                );
            }
            else
            {
                glVertexArrayAttribFormat(
                    _id,
                    attrib.location,
                    componentCount,
                    glType,
                    GL_FALSE,
                    attrib.offset
                );
            }

            glVertexArrayAttribBinding(_id, attrib.location, 0);
        }

    }

    void VertexArray::SetIndexBuffer(const IndexBuffer& buffer)
    {
        if (!buffer.IsValid())
        {
            Debug::LogError("VertexArray::SetIndexBuffer failed : Buffer is invalid!\n");
            return;
        }

        if (!IsValid())
        {
            Debug::LogError("VertexArray::SetIndexBuffer failed : VertexArray is invalid!\n");
            return;
        }

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