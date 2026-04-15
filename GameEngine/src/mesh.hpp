#pragma once

struct Vertex;

namespace Graphics
{
    namespace Resource
    {
        // Holds the three GPU buffer handles needed to draw one piece of geometry.
        // VAO - remembers which buffers are bound and how to read them.
        // VBO - stores vertex data (position, colour, etc.) on the GPU.
        // IBO - stores indices so vertices can be reused across triangles.
        struct Mesh
        {
            uint vao = 0;        // Vertex Array Object  - restores all buffer state on bind
            uint vbo = 0;        // Vertex Buffer Object - raw vertex data on the GPU
            uint ibo = 0;        // Index Buffer Object  - indices into the VBO
            uint indexCount = 0; // number of indices; passed to glDrawElements

            bool Upload(const std::vector<Vertex>& verts, const std::vector<uint>& indices); // uploads data to GPU, sets up VAO
            void Shutdown();                                                                // frees GPU buffers

            bool IsValid() const { return vao != 0; } // vao == 0 means Init() was never called or failed
        };
    }
}