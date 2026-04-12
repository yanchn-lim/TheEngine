#include "mesh.hpp"
#include "vertex.hpp"
#include "debug.hpp"

#include <glad/glad.h>

// -------------------------------------------------------------------
// Mesh
// -------------------------------------------------------------------

bool Mesh::Init(const std::vector<Vertex>& verts, const std::vector<uint>& indices)
{
    indexCount = (uint)indices.size();

    //get a handle id for each buffer
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint), indices.data(), GL_STATIC_DRAW);

    // aPos - location 0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    // aCol - location 1
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

// -------------------------------------------------------------------
// MeshLibrary - built-in generators
// -------------------------------------------------------------------

Mesh MeshLibrary::MakeQuad()
{
    // unit quad centered at origin, in local space
    // scale via model matrix at draw time
    const std::vector<Vertex> verts =
    {
        { float2(-0.5f,  0.5f), float3(1.f, 1.f, 1.f) },  // top-left
        { float2(0.5f,  0.5f), float3(1.f, 1.f, 1.f) },  // top-right
        { float2(0.5f, -0.5f), float3(1.f, 1.f, 1.f) },  // bottom-right
        { float2(-0.5f, -0.5f), float3(1.f, 1.f, 1.f) },  // bottom-left
    };

    const std::vector<uint> indices = { 0, 1, 2, 2, 3, 0 };

    Mesh m;
    m.Init(verts, indices);
    return m;
}

Mesh MeshLibrary::MakeCircle(int segments)
{
    std::vector<Vertex> verts;
    std::vector<uint> indices;

    // center vertex
    verts.push_back({ float2(0.f, 0.f), float3(1.f, 1.f, 1.f) });

    const float step = (2.f * glm::pi<float>()) / (float)segments;

    for (int i = 0; i < segments; ++i)
    {
        float angle = step * (float)i;
        verts.push_back(
            {
                float2(glm::cos(angle) * 0.5f, glm::sin(angle) * 0.5f),
                float3(1.f, 1.f, 1.f)
            });
    }

    // fan triangles from center
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

// -------------------------------------------------------------------
// MeshLibrary
// -------------------------------------------------------------------

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

    _meshes.emplace(name, std::move(mesh));
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