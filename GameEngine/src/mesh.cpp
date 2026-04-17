#include "debug.hpp"

#include "mesh.hpp"
#include "vertex.hpp"

#include <glad/glad.h>

namespace Asset
{
    bool Mesh::Upload(const std::vector<Vertex>& verts, const std::vector<uint>& indices)
    {
        indexCount = (uint)indices.size();

        // Create the GPU objects; these calls fill the uint handles with IDs assigned by OpenGL.
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ibo);

        // Bind the VAO first - every buffer bind and attribute setup below is recorded inside it.
        // Re-binding the VAO later automatically restores all of this state.
        glBindVertexArray(vao);

        // Upload vertex data. GL_STATIC_DRAW hints that the data will not change.
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

        // Upload index data. The IBO binding is also stored inside the VAO.
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint), indices.data(), GL_STATIC_DRAW);

        // Tell OpenGL how to read each field out of the raw vertex bytes.
        // Args: attribute slot, component count, type, normalise, stride, byte offset into Vertex.

        // slot 0 = aPos (vec3)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
        glEnableVertexAttribArray(0);

        // slot 1 = aCol (vec3)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, col));
        glEnableVertexAttribArray(1);

        // slot 2 = aUV  (vec2)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoords));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
        return true;
    }

    void Mesh::Shutdown()
    {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ibo);
        vao = vbo = ibo = 0;
    }
}
