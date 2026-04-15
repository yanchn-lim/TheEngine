#include "debug.hpp"
#include "material.hpp"

namespace Graphics
{
	namespace Resource
	{
		bool MaterialLibrary::Add(AssetHandle handle, Material mat)
		{
			if (Has(handle))
			{
				Debug::LogWarning("MaterialLibrary: '", handle, "' already registered, skipping");
				return true;
			}
	
			_materials.emplace(handle, std::move(mat));
			Debug::CLog("MaterialLibrary: registered '", handle, "'\n");
			return true;
		}
	
		Material& MaterialLibrary::Get(AssetHandle handle)
		{
			assert(_materials.count(handle) && "MaterialLibrary: material not found");
			return _materials.at(handle);
		}
	
		bool MaterialLibrary::Has(AssetHandle handle) const
		{
			return _materials.count(handle) > 0;
		}
	
		void MaterialLibrary::Shutdown()
		{
			_materials.clear();
		}
	}
}
