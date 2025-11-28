#include "renderer2d.hpp"
#include "math.hpp"

#include <glad/glad.h>

namespace Engine
{
    namespace
    {
        struct Renderer2DData
        {
            unsigned int vao = 0;
            unsigned int vbo = 0;
            unsigned int ebo = 0;
            unsigned int shaderProgram = 0;

            int viewProjLocation = -1;
            int modelLocation = -1;
            int colorLocation = -1;

            float4x4 viewProj; // cached per scene
        };

        Renderer2DData s_Data;

        unsigned int CreateShaderProgram(const char* vertexSrc, const char* fragmentSrc)
        {
            unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertexShader, 1, &vertexSrc, nullptr);
            glCompileShader(vertexShader);

            // (Optional) TODO: add error checking

            unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragmentShader, 1, &fragmentSrc, nullptr);
            glCompileShader(fragmentShader);

            // (Optional) TODO: add error checking

            unsigned int program = glCreateProgram();
            glAttachShader(program, vertexShader);
            glAttachShader(program, fragmentShader);
            glLinkProgram(program);

            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);

            return program;
        }
    }
    
    void Renderer2D::Init()
        {
            // --- Geometry: unit quad in local space, centered at origin ---
            // (-0.5,-0.5), (0.5,-0.5), (0.5,0.5), (-0.5,0.5)
            float vertices[] =
            {
                -0.5f, -0.5f, 0.0f,
                 0.5f, -0.5f, 0.0f,
                 0.5f,  0.5f, 0.0f,
                -0.5f,  0.5f, 0.0f
            };

            unsigned int indices[] =
            {
                0, 1, 2,
                2, 3, 0
            };

            // VBO -> Vertex Buffer Object (stores vertex data)
            // VAO -> Vertex Array Object (data to intepret vertex data)
            // sending data to gpu

            glGenVertexArrays(1, &s_Data.vao); // get id from opengl
            glGenBuffers(1, &s_Data.vbo); // get a buffer id from opengl
            glGenBuffers(1, &s_Data.ebo);

            glBindVertexArray(s_Data.vao);

            glBindBuffer(GL_ARRAY_BUFFER, s_Data.vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Data.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(
                0,
                3,
                GL_FLOAT,
                GL_FALSE,
                3 * sizeof(float),
                (void*)0
            );

            glBindVertexArray(0);

            // --- Shader program (2D: viewProj * model * position) ---
            const char* vertexShaderSource = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;

            uniform mat4 uViewProj;
            uniform mat4 uModel;

            void main()
            {
                gl_Position = uViewProj * uModel * vec4(aPos, 1.0);
            }
        )";

            const char* fragmentShaderSource = R"(
            #version 330 core
            out vec4 FragColor;

            uniform vec4 uColor;

            void main()
            {
                FragColor = uColor;
            }
        )";

            s_Data.shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);

            s_Data.viewProjLocation = glGetUniformLocation(s_Data.shaderProgram, "uViewProj");
            s_Data.modelLocation = glGetUniformLocation(s_Data.shaderProgram, "uModel");
            s_Data.colorLocation = glGetUniformLocation(s_Data.shaderProgram, "uColor");
        }

    void Renderer2D::Shutdown()
        {
            if (s_Data.shaderProgram != 0)
            {
                glDeleteProgram(s_Data.shaderProgram);
                s_Data.shaderProgram = 0;
            }

            if (s_Data.ebo != 0)
            {
                glDeleteBuffers(1, &s_Data.ebo);
                s_Data.ebo = 0;
            }

            if (s_Data.vbo != 0)
            {
                glDeleteBuffers(1, &s_Data.vbo);
                s_Data.vbo = 0;
            }

            if (s_Data.vao != 0)
            {
                glDeleteVertexArrays(1, &s_Data.vao);
                s_Data.vao = 0;
            }
        }

    void Renderer2D::BeginScene(const float4x4& viewProj)
        {
            s_Data.viewProj = viewProj;

            glUseProgram(s_Data.shaderProgram);

            if (s_Data.viewProjLocation >= 0)
            {
                glUniformMatrix4fv(
                    s_Data.viewProjLocation,
                    1,
                    GL_FALSE,
                    s_Data.viewProj.m
                );
            }
        }

    void Renderer2D::EndScene()
        {
            glUseProgram(0);
        }

    void Renderer2D::DrawQuad(
        const float3& position,
        const float2& size,
        const float4& color,
        float rotationZ
    )
    {
        glUseProgram(s_Data.shaderProgram);

        // Color
        if (s_Data.colorLocation >= 0)
            {
                glUniform4f(
                    s_Data.colorLocation,
                    color.x,
                    color.y,
                    color.z,
                    color.w
                );
            }

        // Scale from logical size + Z = 1
        float3 scale;
        scale.x = size.x;
        scale.y = size.y;
        scale.z = 1.0f;

        // Build model matrix using your existing math helpers
        float4x4 model = MakeModelMatrix(position, rotationZ, scale);

        if (s_Data.modelLocation >= 0)
        {
            glUniformMatrix4fv(
                s_Data.modelLocation,
                1,
                GL_FALSE,
                model.m
            );
        }

        // ViewProj is already set in BeginScene

        glBindVertexArray(s_Data.vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
}
