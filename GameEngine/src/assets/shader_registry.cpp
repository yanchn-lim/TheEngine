#include "shader_registry.hpp"

#include <cstring>

#include "core/file_system.hpp"
#include "debug/debug.hpp"

namespace
{
    // load optional SPIR-V words while keeping file parsing outside the GPU back end
    bool ReadSpirv(const std::string& path, std::vector<uint32_t>& output)
    {
        if (path.empty())
            return true;

        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
            return false;

        const std::streamsize size = file.tellg();
        if (size <= 0 || size % static_cast<std::streamsize>(sizeof(uint32_t)) != 0)
            return false;

        file.seekg(0, std::ios::beg);
        output.resize(static_cast<size_t>(size) / sizeof(uint32_t));
        return static_cast<bool>(file.read(
            reinterpret_cast<char*>(output.data()), size));
    }
}

namespace Ludus::Assets
{
    ShaderHandle ShaderRegistry::Load(
        const std::string& vertexPath,
        const std::string& fragmentPath,
        const std::string& vertexSpirvPath,
        const std::string& fragmentSpirvPath)
    {
        // use all variant paths as one deduplication key
        const std::string key = vertexPath + "|" + fragmentPath + "|" +
            vertexSpirvPath + "|" + fragmentSpirvPath;
        if (const auto existing = _pathToHandle.find(key); existing != _pathToHandle.end())
            return existing->second;

        // retain GLSL and SPIR-V together so the selected back end uses its native form
        ShaderAsset asset;
        asset.label = key;
        if (!Ludus::FileSystem::ReadTextFile(vertexPath.c_str(), asset.vertexSource) ||
            !Ludus::FileSystem::ReadTextFile(fragmentPath.c_str(), asset.fragmentSource))
        {
            Ludus::Debug::LogError("ShaderRegistry::Load : Failed to read GLSL source for ", key);
            return {};
        }

        if (!ReadSpirv(vertexSpirvPath, asset.vertexSpirv) ||
            !ReadSpirv(fragmentSpirvPath, asset.fragmentSpirv))
        {
            Ludus::Debug::LogError("ShaderRegistry::Load : Failed to read SPIR-V for ", key);
            return {};
        }

        const ShaderHandle handle{ _nextId++ };
        _pathToHandle[key] = handle;
        _shaders.emplace(handle.id, std::move(asset));
        return handle;
    }

    const ShaderAsset* ShaderRegistry::Get(ShaderHandle handle) const
    {
        if (!handle)
            return nullptr;

        const auto found = _shaders.find(handle.id);
        return found == _shaders.end() ? nullptr : &found->second;
    }

    void ShaderRegistry::Clear()
    {
        _nextId = 1;
        _pathToHandle.clear();
        _shaders.clear();
    }
}
