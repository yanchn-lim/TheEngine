#pragma once

#include <string>

#include "graphics/renderer_backend.hpp"

namespace Ludus
{
    struct EngineConfig
    {
        Graphics::RendererBackend rendererBackend = Graphics::RendererBackend::VULKAN;
        int windowWidth = 1600;
        int windowHeight = 900;
        std::string windowTitle = "Ludus";
        bool vsync = false;
    };
}
