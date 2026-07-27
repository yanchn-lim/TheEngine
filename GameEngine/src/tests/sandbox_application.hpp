#pragma once

#include "core/application.hpp"
#include "debug/debug_overlay.hpp"
#include "manual_render_test.hpp"

namespace Tests
{
    class SandboxApplication final : public Ludus::IApplication
    {
    public:
        bool OnInitialize(Ludus::Engine& engine) override;
        void OnUpdate(Ludus::Engine& engine) override;
        void OnImGui(Ludus::Engine& engine) override;
        void OnKey(Ludus::Engine& engine, int key, int action) override;
        void OnShutdown(Ludus::Engine& engine) override;

    private:
        ManualRenderTest _manualRenderTest;
        DebugOverlay _debugOverlay;
    };
}
