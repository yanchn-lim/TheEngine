#pragma once

#include <cstdint>
#include "assets/asset_handle.hpp"
#include "rendering/render_item.hpp"
#include "rendering/render_world.hpp"

struct MeshRendererComponent
{
    // serialized fields describe what the entity renders
    Assets::MeshHandle mesh;
    Assets::MaterialHandle material;
    Rendering::RenderLayerMask layers = Rendering::DefaultRenderLayer;
    bool visible = true;
    // runtime fields connect this component to RenderWorld and are not serialized
    Rendering::RenderInstanceHandle renderInstance;
    uint64_t renderVersion = 0;
};
