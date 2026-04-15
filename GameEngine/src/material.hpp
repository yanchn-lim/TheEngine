#pragma once

#include "graphics_resource.hpp"

namespace Graphics
{
    namespace Resource
    {
        struct Material
        {
            AssetHandle shaderHandle;
            AssetHandle textureHandle;
	        uint layer;
        };

        class MaterialLibrary
        {
        public:
            static MaterialLibrary& Get() { static MaterialLibrary instance; return instance; }

            bool      Add(AssetHandle handle, Material mat);
            Material& Get(AssetHandle handle);
            bool      Has(AssetHandle handle) const;
            void      Shutdown();

        private:
            std::unordered_map<AssetHandle, Material> _materials;
        };
    }
}
