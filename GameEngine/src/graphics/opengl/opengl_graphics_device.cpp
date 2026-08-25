#include "opengl_graphics_device.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#include "debug/debug.hpp"

namespace
{
    // compile one GLSL stage and report its label with any compiler error
    GLuint CompileShader(GLenum stage, const std::string& source, const std::string& label)
    {
        const GLuint shader = glCreateShader(stage);
        const char* text = source.c_str();
        glShaderSource(shader, 1, &text, nullptr);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_TRUE)
            return shader;

        char message[2048]{};
        glGetShaderInfoLog(shader, sizeof(message), nullptr, message);
        Debug::LogError("OpenGL shader compile failed for ", label, ": ", message);
        glDeleteShader(shader);
        return 0;
    }

    GLenum ToTopology(Graphics::PrimitiveTopology topology)
    {
        switch (topology)
        {
        case Graphics::PrimitiveTopology::LINES: return GL_LINES;
        case Graphics::PrimitiveTopology::POINTS: return GL_POINTS;
        default: return GL_TRIANGLES;
        }
    }

    GLenum ToBaseType(Graphics::ShaderDataType type)
    {
        return Graphics::IsShaderDataTypeInteger(type) ? GL_INT : GL_FLOAT;
    }
}

namespace Graphics
{
    bool OpenGLGraphicsDevice::Initialize(const GraphicsDeviceDesc& desc)
    {
        // OpenGL owns context activation, function loading, and swap interval setup
        _window = static_cast<GLFWwindow*>(desc.window);
        if (!_window)
            return false;

        glfwMakeContextCurrent(_window);
        glfwSwapInterval(desc.vsync ? 1 : 0);
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        {
            Debug::LogError("OpenGLGraphicsDevice: GLAD initialization failed");
            return false;
        }

        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glCreateVertexArrays(1, &_vertexArray);
        glBindVertexArray(_vertexArray);
        return _vertexArray != 0;
    }

    GpuBufferHandle OpenGLGraphicsDevice::CreateBuffer(const BufferDesc& desc)
    {
        // immutable CPU data is copied into a device-owned OpenGL buffer
        if (!desc.data || desc.size == 0)
            return {};

        GLuint id = 0;
        glCreateBuffers(1, &id);
        glNamedBufferData(id, static_cast<GLsizeiptr>(desc.size), desc.data, GL_STATIC_DRAW);
        return id ? _buffers.Create(BufferResource{ id }) : GpuBufferHandle{};
    }

    GpuTextureHandle OpenGLGraphicsDevice::CreateTexture(const TextureDesc& desc)
    {
        if (!desc.pixels || desc.width == 0 || desc.height == 0)
            return {};

        GLuint id = 0;
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, 1, GL_RGBA8, static_cast<GLsizei>(desc.width), static_cast<GLsizei>(desc.height));
        glTextureSubImage2D(id, 0, 0, 0, static_cast<GLsizei>(desc.width),
            static_cast<GLsizei>(desc.height), GL_RGBA, GL_UNSIGNED_BYTE, desc.pixels);
        return id ? _textures.Create(TextureResource{ id }) : GpuTextureHandle{};
    }

    GpuSamplerHandle OpenGLGraphicsDevice::CreateSampler(const SamplerDesc& desc)
    {
        GLuint id = 0;
        glCreateSamplers(1, &id);
        glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, desc.linear ? GL_LINEAR : GL_NEAREST);
        glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, desc.linear ? GL_LINEAR : GL_NEAREST);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_S, desc.repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
        glSamplerParameteri(id, GL_TEXTURE_WRAP_T, desc.repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
        return id ? _samplers.Create(SamplerResource{ id }) : GpuSamplerHandle{};
    }

    GpuShaderHandle OpenGLGraphicsDevice::CreateShader(const ShaderProgramDesc& desc)
    {
        // GLSL stages are linked into the program represented by one shader handle
        const GLuint vertex = CompileShader(GL_VERTEX_SHADER, desc.vertexSource, desc.label);
        if (!vertex)
            return {};
        const GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, desc.fragmentSource, desc.label);
        if (!fragment)
        {
            glDeleteShader(vertex);
            return {};
        }

        const GLuint program = glCreateProgram();
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glLinkProgram(program);
        glDeleteShader(vertex);
        glDeleteShader(fragment);

        GLint success = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (success != GL_TRUE)
        {
            char message[2048]{};
            glGetProgramInfoLog(program, sizeof(message), nullptr, message);
            Debug::LogError("OpenGL program link failed for ", desc.label, ": ", message);
            glDeleteProgram(program);
            return {};
        }

        return _shaders.Create(ShaderResource{ program });
    }

    GpuPipelineHandle OpenGLGraphicsDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
    {
        return _shaders.Get(desc.shader) ? _pipelines.Create(PipelineResource{ desc }) : GpuPipelineHandle{};
    }

    void OpenGLGraphicsDevice::DestroyBuffer(GpuBufferHandle handle)
    {
        if (BufferResource* resource = _buffers.Get(handle)) glDeleteBuffers(1, &resource->id);
        _buffers.Destroy(handle);
    }

    void OpenGLGraphicsDevice::DestroyTexture(GpuTextureHandle handle)
    {
        if (TextureResource* resource = _textures.Get(handle)) glDeleteTextures(1, &resource->id);
        _textures.Destroy(handle);
    }

    void OpenGLGraphicsDevice::DestroySampler(GpuSamplerHandle handle)
    {
        if (SamplerResource* resource = _samplers.Get(handle)) glDeleteSamplers(1, &resource->id);
        _samplers.Destroy(handle);
    }

    void OpenGLGraphicsDevice::DestroyShader(GpuShaderHandle handle)
    {
        if (ShaderResource* resource = _shaders.Get(handle)) glDeleteProgram(resource->program);
        _shaders.Destroy(handle);
    }

    void OpenGLGraphicsDevice::DestroyPipeline(GpuPipelineHandle handle)
    {
        _pipelines.Destroy(handle);
    }

    FrameStatus OpenGLGraphicsDevice::BeginFrame()
    {
        _activePipeline = {};
        return FrameStatus::Success;
    }

    FrameStatus OpenGLGraphicsDevice::EndFrame()
    {
        glfwSwapBuffers(_window);
        return FrameStatus::Success;
    }

    void OpenGLGraphicsDevice::OnResize(uint32_t width, uint32_t height)
    {
        glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    }

    void OpenGLGraphicsDevice::WaitIdle() { glFinish(); }

    void OpenGLGraphicsDevice::Shutdown()
    {
        // release escaped resources before the GLFW context is destroyed
        if (!_window) return;
        WaitIdle();
        _buffers.ForEach([](BufferResource& resource) { glDeleteBuffers(1, &resource.id); });
        _textures.ForEach([](TextureResource& resource) { glDeleteTextures(1, &resource.id); });
        _samplers.ForEach([](SamplerResource& resource) { glDeleteSamplers(1, &resource.id); });
        _shaders.ForEach([](ShaderResource& resource) { glDeleteProgram(resource.program); });
        if (_vertexArray) glDeleteVertexArrays(1, &_vertexArray);
        _vertexArray = 0;
        _pipelines.Clear();
        _shaders.Clear();
        _samplers.Clear();
        _textures.Clear();
        _buffers.Clear();
        _window = nullptr;
    }

    void OpenGLGraphicsDevice::BeginRenderPass(const RenderPassDesc& desc)
    {
        // OpenGL starts a pass by clearing the currently bound framebuffer
        GLbitfield flags = 0;
        if (desc.clearColorTarget)
        {
            glClearColor(desc.clearColor.r, desc.clearColor.g, desc.clearColor.b, desc.clearColor.a);
            flags |= GL_COLOR_BUFFER_BIT;
        }
        if (desc.clearDepthTarget)
            flags |= GL_DEPTH_BUFFER_BIT;
        glClear(flags);
    }

    void OpenGLGraphicsDevice::EndRenderPass() {}

    void OpenGLGraphicsDevice::SetViewport(const ViewportDesc& viewport)
    {
        glViewport(static_cast<GLint>(viewport.x), static_cast<GLint>(viewport.y),
            static_cast<GLsizei>(viewport.width), static_cast<GLsizei>(viewport.height));
    }

    void OpenGLGraphicsDevice::SetPipeline(GpuPipelineHandle pipeline)
    {
        PipelineResource* resource = _pipelines.Get(pipeline);
        if (!resource)
            return;
        ShaderResource* shader = _shaders.Get(resource->desc.shader);
        if (!shader)
            return;
        _activePipeline = pipeline;
        glUseProgram(shader->program);
        ApplyRenderState(resource->desc.renderState);
    }

    void OpenGLGraphicsDevice::SetVertexBuffer(GpuBufferHandle buffer, const VertexLayout& layout)
    {
        // the shared layout configures one backend-owned vertex array
        BufferResource* resource = _buffers.Get(buffer);
        if (!resource)
            return;

        glBindVertexArray(_vertexArray);
        glVertexArrayVertexBuffer(_vertexArray, 0, resource->id, 0, static_cast<GLsizei>(layout.stride));
        for (const VertexAttribute& attribute : layout.attributes)
        {
            glEnableVertexArrayAttrib(_vertexArray, attribute.location);
            if (IsShaderDataTypeInteger(attribute.type))
                glVertexArrayAttribIFormat(_vertexArray, attribute.location,
                    GetShaderDataTypeComponentCount(attribute.type), ToBaseType(attribute.type), attribute.offset);
            else
                glVertexArrayAttribFormat(_vertexArray, attribute.location,
                    GetShaderDataTypeComponentCount(attribute.type), ToBaseType(attribute.type), GL_FALSE, attribute.offset);
            glVertexArrayAttribBinding(_vertexArray, attribute.location, 0);
        }
    }

    void OpenGLGraphicsDevice::SetIndexBuffer(GpuBufferHandle buffer)
    {
        if (BufferResource* resource = _buffers.Get(buffer))
            glVertexArrayElementBuffer(_vertexArray, resource->id);
    }

    void OpenGLGraphicsDevice::SetFrameConstants(const FrameConstants& constants)
    {
        const GLuint program = ActiveProgram();
        if (!program) return;
        const GLint view = glGetUniformLocation(program, "uView");
        const GLint projection = glGetUniformLocation(program, "uProjection");
        if (view >= 0) glUniformMatrix4fv(view, 1, GL_FALSE, glm::value_ptr(constants.view));
        if (projection >= 0) glUniformMatrix4fv(projection, 1, GL_FALSE, glm::value_ptr(constants.projection));
    }

    void OpenGLGraphicsDevice::SetMaterialResources(GpuTextureHandle texture, GpuSamplerHandle sampler)
    {
        TextureResource* textureResource = _textures.Get(texture);
        SamplerResource* samplerResource = _samplers.Get(sampler);
        if (!textureResource || !samplerResource)
            return;
        glBindTextureUnit(0, textureResource->id);
        glBindSampler(0, samplerResource->id);
        const GLuint program = ActiveProgram();
        const GLint location = glGetUniformLocation(program, "uTexture");
        if (location >= 0) glUniform1i(location, 0);
    }

    void OpenGLGraphicsDevice::SetDrawConstants(const DrawConstants& constants)
    {
        // the first implementation maps shared constants to known GLSL uniforms
        const GLuint program = ActiveProgram();
        if (!program) return;
        const GLint model = glGetUniformLocation(program, "uModel");
        if (model >= 0) glUniformMatrix4fv(model, 1, GL_FALSE, glm::value_ptr(constants.model));
        const GLint tint = glGetUniformLocation(program, "uTint");
        if (tint >= 0) glUniform4fv(tint, 1, glm::value_ptr(constants.tint));
        const GLint uvRect = glGetUniformLocation(program, "uUvRect");
        if (uvRect >= 0) glUniform4fv(uvRect, 1, glm::value_ptr(constants.uvRect));
    }

    void OpenGLGraphicsDevice::Draw(uint32_t vertexCount)
    {
        const PipelineResource* pipeline = _pipelines.Get(_activePipeline);
        if (pipeline)
            glDrawArrays(ToTopology(pipeline->desc.topology), 0, static_cast<GLsizei>(vertexCount));
    }

    void OpenGLGraphicsDevice::DrawIndexed(uint32_t indexCount)
    {
        const PipelineResource* pipeline = _pipelines.Get(_activePipeline);
        if (pipeline)
            glDrawElements(ToTopology(pipeline->desc.topology), static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
    }

    uint32_t OpenGLGraphicsDevice::ActiveProgram() const
    {
        const PipelineResource* pipeline = _pipelines.Get(_activePipeline);
        if (!pipeline) return 0;
        const ShaderResource* shader = _shaders.Get(pipeline->desc.shader);
        return shader ? shader->program : 0;
    }

    void OpenGLGraphicsDevice::ApplyRenderState(const RenderState& state)
    {
        if (state.depthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        glDepthMask(state.depthWrite ? GL_TRUE : GL_FALSE);
        if (state.culling) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        switch (state.blendMode)
        {
        case BlendMode::NONE: glDisable(GL_BLEND); break;
        case BlendMode::ADDITIVE: glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); break;
        case BlendMode::PREMULTIPLIED_ALPHA: glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); break;
        case BlendMode::MULTIPLY: glEnable(GL_BLEND); glBlendFunc(GL_DST_COLOR, GL_ZERO); break;
        default: glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
        }
        glBlendEquation(GL_FUNC_ADD);
    }
}
