#pragma once

struct Vertex;

struct Mesh
{
	uint vao = 0; //vertex array  object
	uint vbo = 0; //vertex buffer object
	uint ibo = 0; //index  buffer object
	uint indexCount = 0;

	bool Init(const std::vector<Vertex>& verts, const std::vector<uint>& indices);
	void Shutdown();

	bool IsValid() const { return vao != 0; }
};

class MeshLibrary
{
public:
    static MeshLibrary& Get() { static MeshLibrary instance; return instance; }
    MeshLibrary(const MeshLibrary&) = delete;
    MeshLibrary& operator=(const MeshLibrary&) = delete;

    // register a mesh under a name
    bool Add(const std::string& name, Mesh mesh);
    Mesh& Get(const std::string& name);
    bool Has(const std::string& name) const;

    // built-in generators
    static Mesh MakeQuad();
    static Mesh MakeCircle(int segments = 32);

    void Shutdown();

private:
    MeshLibrary() = default;
    std::unordered_map<std::string, Mesh> _meshes;
};