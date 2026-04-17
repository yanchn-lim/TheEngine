#include "shader.hpp"
#include "debug.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include "texture.hpp"

namespace Asset
{
    Shader::Shader(const std::string& vertPath, const std::string& fragPath)
    {
        std::string vertSrc = ReadFile(vertPath);
        std::string fragSrc = ReadFile(fragPath);

        if (vertSrc.empty() || fragSrc.empty())
        {
            Debug::LogError("Shader: failed to read source files\n");
            return;
        }

        // Compile each stage separately, then link them into one program.
        uint vert = CompileStage(vertSrc.c_str(), GL_VERTEX_SHADER);
        uint frag = CompileStage(fragSrc.c_str(), GL_FRAGMENT_SHADER);
        _program = Link(vert, frag);

        // Stage objects are no longer needed once the program is linked.
        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    void Shader::Shutdown()
    {
        glDeleteProgram(_program);
    }

    void Shader::Bind() const
    {
        glUseProgram(_program);
    }

    void Shader::Unbind() const
    {
        glUseProgram(0); // 0 unbinds any active program
    }

    void Shader::SetInt(const char* name, int value) const
    {
        glUniform1i(GetLocation(name), value);
    }

    void Shader::SetFloat(const char* name, float value) const
    {
        glUniform1f(GetLocation(name), value);
    }

    void Shader::SetMat4(const char* name, const mat4& value) const
    {
        // GL_FALSE: do not transpose - GLM and OpenGL both use column-major layout.
        glUniformMatrix4fv(GetLocation(name), 1, GL_FALSE, glm::value_ptr(value));
    }

    void Shader::SetFloat3(const char* name, const float3& value) const
    {
        glUniform3fv(GetLocation(name), 1, glm::value_ptr(value));
    }

    void Shader::SetTexture(const char* name, Texture2D& tex) const
    {
        glBindTextureUnit(0,tex.id);
        glUniform1i(GetLocation(name), 0); // 0 = GL_TEXTURE0
    }

    // -----------------------------------------------------------------------
    // Shader private
    // -----------------------------------------------------------------------
    int Shader::GetLocation(const char* name) const
    {
        // Returns -1 if the uniform does not exist or was optimised away by the GLSL compiler.
        int loc = glGetUniformLocation(_program, name);
        if (loc == -1)
            Debug::LogWarning("Shader: uniform '", name, "' not found or optimized out\n");
        return loc;
    }

    std::string Shader::ReadFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            Debug::LogError("Shader: could not open file '", path, "'\n");
            return {};
        }

        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    uint Shader::CompileStage(const char* src, uint type)
    {
        uint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr); // upload GLSL source text; nullptr = null-terminated string
        glCompileShader(shader);

        int ok;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            char log[512];
            glGetShaderInfoLog(shader, 512, nullptr, log);
            const char* typeName = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
            Debug::LogError("Shader: ", typeName, " compile error: ", log);
        }

        return shader;
    }

    uint Shader::Link(uint vert, uint frag)
    {
        uint prog = glCreateProgram();
        glAttachShader(prog, vert);
        glAttachShader(prog, frag);
        glLinkProgram(prog); // resolves connections between stages (e.g. vertex out -> fragment in)

        int ok;
        glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            char log[512];
            glGetProgramInfoLog(prog, 512, nullptr, log);
            Debug::LogError("Shader: link error: ", log);
            return 0;
        }

        return prog;
    }
}