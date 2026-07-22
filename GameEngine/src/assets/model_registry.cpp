#include "model_registry.hpp"
#include "model_asset.hpp"
#include "debug/debug.hpp"

namespace Assets
{
	ModelHandle ModelRegistry::Create(const std::string& name, std::vector<MeshHandle> handles)
	{
		// reuse named models so repeated loads keep stable asset references
		const auto it = _nameToHandle.find(name);
		if (it != _nameToHandle.end())
			return it->second;

		if (handles.empty())
		{
			Debug::LogError("ModelRegistry::Create : List of handles is empty!");
			return ModelHandle();
		}

		ModelHandle handle{ _nextId++ };

		ModelAsset model{std::move(handles)};
		_nameToHandle[name] = handle;
		_models[handle.id] = model;

		return handle;
	}

	const ModelAsset* ModelRegistry::Get(ModelHandle handle) const
	{
		if (!handle)
		{
			Debug::LogError("ModelRegistry::Get : ModelHandle [", handle.id, "] is invalid");
			return nullptr;
		}

		const auto it = _models.find(handle.id);
		if (it == _models.end())
		{
			Debug::LogError("ModelRegistry::Get : Could not find ModelHandle [", handle.id, "] in the registry");
			return nullptr;
		}

		return &it->second;
	}

	const ModelAsset* ModelRegistry::Get(const std::string& handle) const
	{
		const auto it = _nameToHandle.find(handle);
		if (it == _nameToHandle.end())
		{
			Debug::LogError("ModelRegistry::Get : ", handle, " could not be found in the registry");
			return nullptr;
		}

		return Get(it->second);
	}

	void ModelRegistry::Clear()
	{
		// discard groupings without affecting meshes owned by MeshRegistry
		_nextId = 1;
		_nameToHandle.clear();
		_models.clear();
	}
}
