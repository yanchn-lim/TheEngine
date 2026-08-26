#pragma once

#include "core/application.hpp"
#include "debug/debug_overlay.hpp"
#include "scene/scene.hpp"

namespace Tests
{
    class SandboxApplication final : public Ludus::IApplication
    {
    public:
        bool OnInitialize(Ludus::Engine& engine) override;
        void OnFixedUpdate(Ludus::Engine& engine, double fixedDeltaTime) override;
        void OnUpdate(Ludus::Engine& engine) override;
        void OnImGui(Ludus::Engine& engine) override;
        void OnKey(Ludus::Engine& engine, int key, int action) override;
    private:
		bool LoadScene(Ludus::Engine& engine, const char* path);

        DebugOverlay _debugOverlay;
        Ludus::Scene _scene;
    };
}
