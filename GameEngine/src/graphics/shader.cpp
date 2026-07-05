#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"
#include "debug/debug.hpp"

namespace Graphics
{
	uint32_t Shader::_compileshader(uint32_t type, const std::string& source)
	{
		// Compile one shader stage and return 0 on failure.
		const char* src = source.c_str();

		const uint32_t shader = glCreateShader(type);
		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);

		int success = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

		if (!success)
		{
			char infoLog[1024]{};
			glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
			Debug::LogError("Shader compile failed: ", infoLog);
			glDeleteShader(shader);
			return 0;
		}

		return shader;
	}

	bool Shader::Create(const std::string& vertsrc, const std::string& fragsrc)
	{
		// Compile, link, and own a complete GPU shader program.
		Destroy();

		uint32_t vert = _compileshader(GL_VERTEX_SHADER, vertsrc);
		if (0 == vert)
		{
			Debug::LogError("Shader::Create failed: vertex shader compilation failed");
			return false;
		}
		
		uint32_t frag = _compileshader(GL_FRAGMENT_SHADER, fragsrc);
		if (0 == frag)
		{
			Debug::LogError("Shader::Create failed: fragment shader compilation failed");
			glDeleteShader(vert);
			return false;
		}

		uint32_t program = glCreateProgram();
		glAttachShader(program, vert);
		glAttachShader(program, frag);
		glLinkProgram(program);

		glDeleteShader(vert);
		glDeleteShader(frag);

		int success = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success)
		{
			char infoLog[1024]{};
			glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
			Debug::LogError("Shader link failed: ", infoLog);
			glDeleteProgram(program);
			return false;
		}

		_id = program;
		return true;
	}

	void Shader::Bind() const
	{
		// Binding an invalid program is skipped so renderer errors stay visible.
		if (_id == 0)
		{
			Debug::LogError("Shader is invalid!");
			return;
		}

		glUseProgram(_id);
	}

	void Shader::Destroy()
	{
		// Shader owns exactly one OpenGL program id.
		if (_id != 0)
		{
			glDeleteProgram(_id);
			_id = 0;
		}
	}

	void Shader::SetInt(const char* name, const int val) const
	{
		// Uniform setters intentionally validate lookup to catch shader mismatches.
		if (!IsValid())
		{
			Debug::LogError("Shader::SetInt failed: shader is invalid");
			return;
		}

		GLint loc = glGetUniformLocation(_id, name);

		if (loc == -1)
		{
			Debug::LogError("Shader uniform location not found!");
			return;
		}

		glUniform1i(loc, val);
	}

	void Shader::SetMat4(const char* name, const glm::mat4& val) const
	{
		// Matrices are uploaded in GLM's column-major memory layout.
		if (!IsValid())
		{
			Debug::LogError("Shader::SetMat4 failed: shader is invalid");
			return;
		}

		GLint loc = glGetUniformLocation(_id, name);

		if (loc == -1)
		{
			Debug::LogError("Shader uniform location not found!");
			return;
		}

		glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(val));
	}


	uint32_t Shader::GetId() const
	{
		return _id;
	}

	bool Shader::IsValid() const
	{
		return _id != 0;
	}

	Shader::~Shader()
	{
		Destroy();
	}

	Shader::Shader(Shader&& oth) noexcept : _id(oth._id)
	{
		// Transfer program ownership without duplicating the GL object.
		oth._id = 0;
	}

	Shader& Shader::operator=(Shader&& oth) noexcept
	{
		// Replace the current program with ownership from another shader.
		if (&oth == this) return *this;

		Destroy();

		_id = oth._id;
		oth._id = 0;
		return *this;
	}
}
