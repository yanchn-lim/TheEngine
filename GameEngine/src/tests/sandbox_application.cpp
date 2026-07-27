#include "sandbox_application.hpp"

#include "core/engine.hpp"
#include "debug/debug.hpp"
#include "graphics_api_tests.hpp"
#include "rendering/renderer.hpp"

namespace Tests
{
    bool SandboxApplication::OnInitialize(Ludus::Engine& engine)
    {
#ifndef NDEBUG
        if (!RunGraphicsApiTests())
        {
            Debug::LogError("Graphics API self-tests failed");
            return false;
        }
#endif

        if (!_manualRenderTest.Initialize(engine.GetAssets(), engine.GetRenderWorld()))
            return false;

        engine.GetRenderer().Configure({
            _manualRenderTest.GetSpriteMesh(),
            _manualRenderTest.GetMaterial()
        });
        return true;
    }

    void SandboxApplication::OnUpdate(Ludus::Engine& engine)
    {
        _manualRenderTest.Update(engine.GetRenderWorld(), engine.GetTime().deltaTime);
    }

    void SandboxApplication::OnImGui(Ludus::Engine&)
    {
        _debugOverlay.Draw();
    }

    void SandboxApplication::OnKey(Ludus::Engine&, int key, int action)
    {
        _debugOverlay.HandleKey(key, action);
    }

    void SandboxApplication::OnShutdown(Ludus::Engine& engine)
    {
        _manualRenderTest.Shutdown(engine.GetRenderWorld());
    }
}
