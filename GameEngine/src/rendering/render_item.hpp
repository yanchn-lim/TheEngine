#pragma once

#include <glm/glm.hpp>

#include "assets/asset_handle.hpp"

namespace Ludus::Rendering
{
    struct MeshInstanceDesc
    {
        Ludus::Assets::MeshHandle mesh;
        Ludus::Assets::MaterialHandle materialOverride;
        glm::mat4 transform{ 1.0f };
    };
}
