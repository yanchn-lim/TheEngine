#pragma once

#include <vector>

#include "assets/asset_handle.hpp"
#include "rendering/render_world.hpp"

namespace Assets
{
    class AssetManager;
}

namespace Tests
{
    class ManualRenderTest
    {
    public:
        bool Initialize(Assets::AssetManager& assets, Rendering::RenderWorld& renderWorld);
        void Update(Rendering::RenderWorld& renderWorld, double deltaTime);
        void Shutdown(Rendering::RenderWorld& renderWorld);

        Assets::MeshHandle GetSpriteMesh() const noexcept;
        Assets::MaterialHandle GetMaterial() const noexcept;

    private:
        Assets::ModelHandle _model;
        Assets::MaterialHandle _material;
        Assets::MeshHandle _spriteMesh;
        std::vector<Rendering::RenderInstanceHandle> _instances;
        float _rotation = 0.0f;
    };
}
