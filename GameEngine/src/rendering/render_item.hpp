#pragma once

#include <glm/glm.hpp>

#include "assets/asset_handle.hpp"

namespace Rendering
{
    struct MeshInstanceDesc
    {
        Assets::MeshHandle mesh;
        Assets::MaterialHandle material;
        glm::mat4 transform{ 1.0f };
    };
}
