#pragma once

namespace Ludus
{
    class Engine;

    class IApplication
    {
    public:
        virtual ~IApplication() = default;

        virtual bool OnInitialize(Engine&) { return true; }
        virtual void OnUpdate(Engine&) {}
        virtual void OnImGui(Engine&) {}
        virtual void OnKey(Engine&, int, int) {}
        virtual void OnShutdown(Engine&) {}
    };
}
