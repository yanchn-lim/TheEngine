#pragma once


namespace Graphics
{
    namespace Resource
    {
        struct Texture;

        // Wraps one linked OpenGL shader program (vertex + fragment stages compiled and linked together).
        // Bind() installs it on the GPU; the Set* methods write uniform values before a draw call.
        class Shader
        {
        public:
            Shader() = default;
            Shader(const std::string& vertPath, const std::string& fragPath); // reads source, compiles, and links
            void Shutdown();

            void Bind()   const; // make this program active for subsequent draw calls
            void Unbind() const; // revert to no program

            // Upload a value to a named uniform variable in the GLSL source.
            void SetInt   (const char* name, int           value) const;
            void SetFloat (const char* name, float         value) const;
            void SetMat4  (const char* name, const mat4&   value) const;
            void SetFloat3(const char* name, const float3& value) const;
            void SetTexture(const char* name, Texture& tex) const;

            bool IsValid() const { return _program != 0; } // 0 means construction failed

        private:
            uint _program = 0; // OpenGL handle to the linked program

            int GetLocation(const char* name) const;           // resolves uniform name to slot index; warns if not found
            static std::string ReadFile(const std::string& path);
            static uint CompileStage(const char* src, uint type); // compiles one shader stage; type is GL_VERTEX_SHADER etc.
            static uint Link(uint vert, uint frag);               // attaches stages and links; returns 0 on failure
        };
    }
}
