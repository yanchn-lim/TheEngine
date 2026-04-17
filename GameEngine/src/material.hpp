#pragma once


#include "asset_types.hpp"

namespace Asset
{
    class Material
    {
    public:
        AssetHandle shaderHandle;
        AssetHandle textureHandle;
        uint layer;
    };
}

