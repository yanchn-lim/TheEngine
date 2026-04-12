#pragma once

struct Vertex;

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

    bool Init(const std::vector<Vertex>& verts, const std::vector<uint>& indices); // uploads data to GPU, sets up VAO
    void Shutdown();                                                                // frees GPU buffers

    bool IsValid() const { return vao != 0; } // vao == 0 means Init() was never called or failed
};

// Central registry that owns all meshes by name.
// Also provides built-in geometry generators.
class MeshLibrary
{
public:
    static MeshLibrary& Get() { static MeshLibrary instance; return instance; }
    MeshLibrary(const MeshLibrary&) = delete;
    MeshLibrary& operator=(const MeshLibrary&) = delete;

    bool  Add(const std::string& name, Mesh mesh); // register a mesh; fails if name exists or mesh is invalid
    Mesh& Get(const std::string& name);            // retrieve by name; asserts if not found
    bool  Has(const std::string& name) const;

    static Mesh MakeQuad();                  // unit quad centered at origin; scale via model matrix
    static Mesh MakeCircle(int segments = 32); // triangle-fan circle; more segments = smoother

    void Shutdown(); // calls Shutdown() on all registered meshes

private:
    MeshLibrary() = default;
    std::unordered_map<std::string, Mesh> _meshes;
};
