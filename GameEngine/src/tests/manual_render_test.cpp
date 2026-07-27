#include "manual_render_test.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "assets/asset_manager.hpp"
#include "assets/primitives/primitive_mesh2d.hpp"

namespace Tests
{
    bool ManualRenderTest::Initialize(Assets::AssetManager& assetManager, Rendering::RenderWorld& world)
    {
        const Assets::ShaderHandle shader = assetManager.LoadShader(
            "assets/shaders/standard_gl.vert", "assets/shaders/standard_gl.frag",
            "assets/shaders/standard_vk.vert.spv", "assets/shaders/standard_vk.frag.spv");
        const Assets::TextureHandle texture = assetManager.LoadTexture("assets/textures/maxwell.png");
        if (!shader || !texture)
            return false;

        _material = assetManager.CreateMaterial("standard_material", shader, texture,
            { false, false, Graphics::BlendMode::NONE, true });
        _model = assetManager.LoadModel("manual_model", "assets/models/maxwell.obj");
        _spriteMesh = assetManager.CreateMesh("builtin_quad", Assets::Primitive2D::Quad());
        const Assets::ModelAsset* modelAsset = assetManager.Get(_model);
        if (!_material || !modelAsset || !_spriteMesh)
            return false;

        for (Assets::MeshHandle mesh : modelAsset->meshes)
            _instances.push_back(world.CreateMeshInstance({ mesh, _material }));
        return true;
    }

    void ManualRenderTest::Update(Rendering::RenderWorld& world, double deltaTime)
    {
        const glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(0.002f)) *
            glm::rotate(glm::mat4(1.0f), _rotation, glm::vec3(0.0f, 1.0f, 0.0f));
        for (Rendering::RenderInstanceHandle instance : _instances)
            world.SetTransform(instance, transform);
        _rotation += 2.0f * static_cast<float>(deltaTime);
    }

    void ManualRenderTest::Shutdown(Rendering::RenderWorld& world)
    {
        for (Rendering::RenderInstanceHandle instance : _instances)
            world.Destroy(instance);
        _instances.clear();
    }

    Assets::MeshHandle ManualRenderTest::GetSpriteMesh() const noexcept
    {
        return _spriteMesh;
    }

    Assets::MaterialHandle ManualRenderTest::GetMaterial() const noexcept
    {
        return _material;
    }
}
