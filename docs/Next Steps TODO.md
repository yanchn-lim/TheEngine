# Next Steps TODO

Use this as the short working checklist after the current OBJ importer progress.

## 1. Runtime Model Asset

Status: done.

Current shape:

```cpp
struct ModelHandle
{
    AssetId id = InvalidAssetId;

    bool IsValid() const;
    explicit operator bool() const;
};

struct ModelAsset
{
    std::vector<MeshHandle> meshes;
};
```

Keep it asset-side. Do not put model ownership in graphics.

Why:

- A model file can contain multiple meshes.
- Later, the model can also hold material slots, mesh names, and node transforms.
- Callers should ask for one model asset instead of manually owning a mesh handle vector.

## 2. `ModelRegistry`

Status: done.

Goal: give model assets the same registry pattern as textures, shaders, materials, and meshes.

Expected responsibilities:

- Own `ModelAsset` objects.
- Return `ModelHandle`.
- Look up `ModelAsset` by handle.
- Optionally cache by name or path.
- Clear models before clearing meshes, because models reference mesh handles.

Current shape:

```cpp
class ModelRegistry
{
public:
    ModelHandle Create(const std::string& name, std::vector<MeshHandle> meshes);
    const ModelAsset* Get(ModelHandle handle) const;
    void Clear();

private:
    AssetId _nextId = 1;
    std::unordered_map<AssetId, ModelAsset> _models;
    std::unordered_map<std::string, ModelHandle> _nameToHandle;
};
```

Keep this registry independent from OBJ/tinyobjloader.

## 3. `AssetManager::LoadModel()` Direction

Status: done.

```cpp
ModelHandle LoadModel(const std::string& name, const std::string& path);
const ModelAsset* Get(ModelHandle handle) const;
```

Current flow:

1. Use `ModelImporterRegistry` to load `ModelImportData`.
2. Convert each `MeshImportData` into a `MeshHandle` through `MeshRegistry`.
3. Store the resulting mesh handles in `ModelRegistry`.
4. Return the `ModelHandle`.

Keep `LoadMesh(name, path)` as a convenience function that loads only the first mesh.

## 4. Move Manual OBJ Tests Out of Permanent Startup Flow

Status: done.

Goal: keep `engine.cpp` from becoming a pile of one-off tests.

Current issue:

- Public OBJ model loading is useful, but it is hardcoded into startup/render flow.
- This makes normal engine boot depend on whatever model test is currently active.

Better direction:

- Add a clearly named manual test section/function.
- Keep only one active manual graphics/asset test at a time.
- Make model test variables easy to remove later.
- Do not build scene/game object architecture around this test code.

Good temporary structure:

```cpp
bool InitializeManualAssetTest(Assets::AssetManager& assets);
void SubmitManualAssetTest(Graphics::Renderer& renderer, const Assets::AssetManager& assets);
```

This is not a final test framework. It is just a cleaner place for manual rendering checks while the engine is still early.

## Notes

- Do not add OBJ material/MTL support yet.
- Do not add persistent asset UIDs yet.
- Keep runtime handles for now.
- Persistent UIDs become important when scene files, prefab files, material files, or editor asset browsing exist.
