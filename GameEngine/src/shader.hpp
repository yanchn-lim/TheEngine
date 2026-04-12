#pragma once

// Wraps one linked OpenGL shader program (vertex + fragment stages compiled and linked together).
// Bind() installs it on the GPU; the Set* methods write uniform values before a draw call.
class Shader
{
public:
    Shader() = default;
    Shader(const std::string& vertPath, const std::string& fragPath); // reads source, compiles, and links

    void Bind()   const; // make this program active for subsequent draw calls
    void Unbind() const; // revert to no program

    // Upload a value to a named uniform variable in the GLSL source.
    void SetInt   (const char* name, int           value) const;
    void SetFloat (const char* name, float         value) const;
    void SetMat4  (const char* name, const mat4&   value) const;
    void SetFloat3(const char* name, const float3& value) const;

    bool IsValid() const { return _program != 0; } // 0 means construction failed

private:
    uint _program = 0; // OpenGL handle to the linked program

    int GetLocation(const char* name) const;           // resolves uniform name to slot index; warns if not found
    static std::string ReadFile(const std::string& path);
    static uint CompileStage(const char* src, uint type); // compiles one shader stage; type is GL_VERTEX_SHADER etc.
    static uint Link(uint vert, uint frag);               // attaches stages and links; returns 0 on failure
};


// Central registry that owns all shader programs by name.
class ShaderLibrary
{
public:
    static ShaderLibrary& Get() { static ShaderLibrary instance; return instance; }
    ShaderLibrary(const ShaderLibrary&) = delete;
    ShaderLibrary& operator=(const ShaderLibrary&) = delete;

    bool    Load(const std::string& name, const std::string& vertPath, const std::string& fragPath); // compile, link, and register
    Shader& Get(const std::string& name);  // retrieve by name; asserts if not found
    bool    Has(const std::string& name) const;

private:
    ShaderLibrary() = default;
    std::unordered_map<std::string, Shader> _shaders;
};
