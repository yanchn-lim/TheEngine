#pragma once

namespace Ludus
{
    class Engine;

    class IApplication
    {
    public:
        virtual ~IApplication() = default;

        virtual bool OnInitialize(Engine&) { return true; }
        virtual void OnFixedUpdate(Engine&, double) {}
        virtual void OnUpdate(Engine&) {}
        virtual void OnImGui(Engine&) {}
        virtual void OnShutdown(Engine&) {}
    };
}
