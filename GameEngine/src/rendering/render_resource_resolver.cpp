#include "render_resource_resolver.hpp"

namespace Rendering
{
    RenderResourceResolver::RenderResourceResolver(const Assets::AssetManager& assets) : _assets(assets)
    {

    }

    const Assets::ModelAsset* RenderResourceResolver::Resolve(Assets::ModelHandle handle) const
    {
        return _assets.Get(handle);
    }

    bool RenderResourceResolver::TryResolve(const RenderItem& item, Graphics::DrawCmd& outDrawCmd) const
    {
        auto mesh = _assets.Get(item.mesh);
        auto material = _assets.Get(item.material);
        if (!mesh || !material)
        {
            outDrawCmd = {};
            return false;
        }

        outDrawCmd.mesh = mesh;
        outDrawCmd.material = material;
        outDrawCmd.transform = item.transform;

        return true;
    }
}