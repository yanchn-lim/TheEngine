#pragma once

#include <memory>

#include "graphics_device.hpp"
#include "renderer_backend.hpp"

namespace Ludus::Graphics
{
    // creates the backend selected before window initialization
    std::unique_ptr<IGraphicsDevice> CreateGraphicsDevice(RendererBackend backend);
}
