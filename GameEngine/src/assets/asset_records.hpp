#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "asset_handle.hpp"
#include "mesh_import_data.hpp"
#include "graphics/render_state.hpp"

namespace Ludus::Assets
{
    // asset records keep CPU data independent from the selected graphics backend
    struct MeshAsset
    {
        std::vector<MeshSurface> surfaces;
        std::string label;
        uint64_t version = 1;
    };

    struct TextureAsset
    {
        std::vector<unsigned char> pixels;
        uint32_t width = 0;
        uint32_t height = 0;
        std::string label;
        uint64_t version = 1;
    };

    struct ShaderAsset
    {
        std::string vertexSource;
        std::string fragmentSource;
        std::vector<uint32_t> vertexSpirv;
        std::vector<uint32_t> fragmentSpirv;
        std::string label;
        uint64_t version = 1;
    };

    // materials reference other assets through handles instead of graphics pointers
    struct MaterialAsset
    {
        ShaderHandle shader;
        TextureHandle texture;
        Ludus::Graphics::RenderState state{};
        std::string label;
        uint64_t version = 1;
    };
}
