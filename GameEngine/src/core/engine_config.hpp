#pragma once

#include <string>

#include "graphics/renderer_backend.hpp"

namespace Ludus
{
    struct EngineConfig
    {
        Ludus::Graphics::RendererBackend rendererBackend = Ludus::Graphics::RendererBackend::VULKAN;
        int windowWidth = 1600;
        int windowHeight = 900;
        std::string windowTitle = "Ludus";
        bool vsync = false;
        double fixedTimeStep = 1.0 / 60.0;
    };
}
