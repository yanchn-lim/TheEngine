#pragma once

#include <cstdint>
#include <variant>

#include <glm/glm.hpp>

#include "assets/asset_handle.hpp"

namespace Rendering
{
    // layer masks let one view reject items that do not belong to it
    using RenderLayerMask = uint32_t;
    inline constexpr RenderLayerMask DefaultRenderLayer = 1u;

    struct MeshInstanceDesc
    {
        Assets::MeshHandle mesh;
        Assets::MaterialHandle material;
        glm::mat4 transform{ 1.0f };
        RenderLayerMask layers = DefaultRenderLayer;
        int32_t sortingOrder = 0;
        uint64_t entityId = 0;
        bool visible = true;
    };

    // sprite data uses the same lifetime and submission rules as mesh data
    struct SpriteInstanceDesc
    {
        Assets::TextureHandle texture;
        Assets::MaterialHandle material;
        glm::mat4 transform{ 1.0f };
        glm::vec4 tint{ 1.0f };
        glm::vec4 uvRect{ 0.0f, 0.0f, 1.0f, 1.0f };
        RenderLayerMask layers = DefaultRenderLayer;
        int32_t sortingOrder = 0;
        uint64_t entityId = 0;
        bool visible = true;
    };

    // one variant keeps persistent and transient submission on one renderer path
    using RenderItem = std::variant<MeshInstanceDesc, SpriteInstanceDesc>;
}
