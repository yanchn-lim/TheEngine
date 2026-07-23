#include "graphics_device_factory.hpp"

#include "opengl/opengl_graphics_device.hpp"
#include "vulkan/vulkan_graphics_device.hpp"

namespace Graphics
{
    std::unique_ptr<IGraphicsDevice> CreateGraphicsDevice(RendererBackend backend)
    {
        // contain concrete back-end construction at the application boundary
        switch (backend)
        {
        case RendererBackend::OPENGL: return std::make_unique<OpenGLGraphicsDevice>();
        case RendererBackend::VULKAN: return std::make_unique<VulkanGraphicsDevice>();
        default: return nullptr;
        }
    }
}
