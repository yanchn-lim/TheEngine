#include "pch.hpp"

#include "core/engine.hpp"
#include "tests/sandbox_application.hpp"
#include "tests/test_runner.hpp"

int main(int argc, char** argv)
{
    Ludus::EngineConfig config;
    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument == "--opengl")
            config.rendererBackend = Graphics::RendererBackend::OPENGL;
        else if (argument == "--vulkan")
            config.rendererBackend = Graphics::RendererBackend::VULKAN;
		else if (argument == "--tests")
			return Tests::RunAllTests();
    }

    Ludus::Engine engine(std::move(config));
    Tests::SandboxApplication application;
    return engine.Run(application);
}
