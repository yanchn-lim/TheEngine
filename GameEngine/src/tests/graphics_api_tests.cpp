#include "graphics_api_tests.hpp"

#include <vector>

#include "graphics/graphics_handles.hpp"
#include "graphics/resource_table.hpp"
#include "rendering/render_world.hpp"

namespace Tests
{
    bool RunGraphicsApiTests()
    {
        Graphics::ResourceTable<Graphics::GpuBufferHandle, int> first;
        Graphics::ResourceTable<Graphics::GpuBufferHandle, int> second;
        const Graphics::GpuBufferHandle original = first.Create(10);
        if (!original || !first.Get(original) || second.Get(original)) return false;
        if (!first.Destroy(original) || first.Get(original) || first.Destroy(original)) return false;
        const Graphics::GpuBufferHandle replacement = first.Create(20);
        if (!replacement || replacement == original || first.Get(original)) return false;

        Rendering::RenderWorld world;
        Rendering::MeshInstanceDesc mesh;
        mesh.mesh = Assets::MeshHandle{ 1 };
        const Rendering::RenderInstanceHandle persistent = world.CreateMeshInstance(mesh);
        world.DrawMeshOnce(mesh);
        std::vector<Rendering::RenderItem> items;
        world.Collect(Rendering::DefaultRenderLayer, items);
        if (items.size() != 2 || !world.Destroy(persistent) || world.SetVisible(persistent, true)) return false;
        world.EndFrame();
        world.Collect(Rendering::DefaultRenderLayer, items);
        return items.empty();
    }
}
