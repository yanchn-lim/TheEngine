#include "mesh.hpp"
#include "vertex.hpp"
#include "debug.hpp"

#include <glad/glad.h>

// -----------------------------------------------------------------------
// Mesh
// -----------------------------------------------------------------------

bool Mesh::Init(const std::vector<Vertex>& verts, const std::vector<uint>& indices)
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

    // slot 0 = aPos (vec2)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    // slot 1 = aCol (vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, col));
    glEnableVertexAttribArray(1);

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

// -----------------------------------------------------------------------
// MeshLibrary - built-in generators
// -----------------------------------------------------------------------

Mesh MeshLibrary::MakeQuad()
{
    // 1x1 square centered at origin. Scale to desired size via the model matrix.
    const std::vector<Vertex> verts =
    {
        { float2(-0.5f,  0.5f), float3(1.f, 1.f, 1.f) },  // top-left
        { float2( 0.5f,  0.5f), float3(1.f, 1.f, 1.f) },  // top-right
        { float2( 0.5f, -0.5f), float3(1.f, 1.f, 1.f) },  // bottom-right
        { float2(-0.5f, -0.5f), float3(1.f, 1.f, 1.f) },  // bottom-left
    };

    // Two triangles: (top-left, top-right, bottom-right) and (bottom-right, bottom-left, top-left).
    const std::vector<uint> indices = { 0, 1, 2, 2, 3, 0 };

    Mesh m;
    m.Init(verts, indices);
    return m;
}

Mesh MeshLibrary::MakeCircle(int segments)
{
    std::vector<Vertex> verts;
    std::vector<uint> indices;

    verts.push_back({ float2(0.f, 0.f), float3(1.f, 1.f, 1.f) }); // center vertex (hub of the fan)

    const float step = (2.f * glm::pi<float>()) / (float)segments;

    // Place one vertex per segment evenly around the circumference (radius 0.5).
    for (int i = 0; i < segments; ++i)
    {
        float angle = step * (float)i;
        verts.push_back(
            {
                float2(glm::cos(angle) * 0.5f, glm::sin(angle) * 0.5f),
                float3(1.f, 1.f, 1.f)
            });
    }

    // Each triangle connects the center to two adjacent rim vertices.
    // Modulo wraps the last triangle back to vertex 1 to close the circle.
    for (int i = 1; i <= segments; ++i)
    {
        indices.push_back(0);
        indices.push_back((uint)i);
        indices.push_back((uint)(i % segments) + 1);
    }

    Mesh m;
    m.Init(verts, indices);
    return m;
}

// -----------------------------------------------------------------------
// MeshLibrary
// -----------------------------------------------------------------------

bool MeshLibrary::Add(const std::string& name, Mesh mesh)
{
    if (Has(name))
    {
        Debug::LogWarning("MeshLibrary: '", name, "' already registered, skipping");
        return true;
    }

    if (!mesh.IsValid())
    {
        Debug::LogError("MeshLibrary: tried to add invalid mesh '", name, "'");
        return false;
    }

    _meshes.emplace(name, std::move(mesh)); // std::move transfers ownership without copying GPU handles
    Debug::Log("MeshLibrary: registered '", name, "'");
    return true;
}

Mesh& MeshLibrary::Get(const std::string& name)
{
    assert(_meshes.count(name) && "MeshLibrary: mesh not found");
    return _meshes.at(name);
}

bool MeshLibrary::Has(const std::string& name) const
{
    return _meshes.count(name) > 0;
}

void MeshLibrary::Shutdown()
{
    for (auto& [name, mesh] : _meshes)
        mesh.Shutdown();
    _meshes.clear();
}
