#include "debug/debug.hpp"
#include "material_registry.hpp"

namespace Assets
{
	MaterialHandle MaterialRegistry::Create(const std::string& name, ShaderHandle shader, TextureHandle texture, Graphics::RenderState state)
	{
		const auto it = _nameToHandle.find(name);
		if (it != _nameToHandle.end())
			return it->second;

		if (!shader || !texture)
		{
			Debug::LogError("MaterialRegistry::Create : Shader or texture handle is invalid");
			return MaterialHandle();
		}

		MaterialHandle handle{ _nextId++ };

		MaterialAsset material { shader, texture, state, name };
		_nameToHandle[name] = handle;
		_materials[handle.id] = material;

		return handle;
	}

	const MaterialAsset* MaterialRegistry::Get(MaterialHandle handle) const
	{
		// handle lookup keeps material ownership centralized in the registry
		if (!handle)
		{
			Debug::LogError("MaterialRegistry::Get : MaterialHandle [", handle.id, "] is invalid");
			return nullptr;
		}

		const auto it = _materials.find(handle.id);
		if (it == _materials.end())
		{
			Debug::LogError("MaterialRegistry::Get : Could not find MaterialHandle [", handle.id, "] in the registry");
			return nullptr;
		}

		return &it->second;
	}

	const MaterialAsset* MaterialRegistry::Get(const std::string& handle) const
	{
		// name lookup is a convenience path for tests and simple engine code
		const auto it = _nameToHandle.find(handle);
		if (it == _nameToHandle.end())
		{
			Debug::LogError("MaterialRegistry::Get : ", handle, " could not be found in the registry");
			return nullptr;
		}

		return Get(it->second);
	}

	void MaterialRegistry::Clear()
	{
		// clear materials before their referenced shader and texture registries are reset
		_nextId = 1;
		_nameToHandle.clear();
		_materials.clear();
	}

}
