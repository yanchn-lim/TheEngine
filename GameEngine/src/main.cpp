#include "pch.hpp"

#include "core/engine.hpp"


int main(int argc, char** argv)
{
    Engine& engine = Engine::Get();
    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument == "--opengl") engine.renderbackend = Graphics::RendererBackend::OPENGL;
        else if (argument == "--vulkan") engine.renderbackend = Graphics::RendererBackend::VULKAN;
    }
    return engine.Run();
}
