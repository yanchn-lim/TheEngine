#pragma once

namespace Ludus
{
    class Engine;

    class IApplication
    {
    public:
        virtual ~IApplication() = default;

    // defines application work at each stage of the engine-owned lifecycle
        virtual bool OnInitialize(Engine&) { return true; }
        virtual void OnFixedUpdate(Engine&, double) {}
        virtual void OnUpdate(Engine&) {}
        virtual void OnImGui(Engine&) {}
        virtual void OnShutdown(Engine&) {}
        // runs once after the window, assets, and renderer are available
    };

        // can run zero or more times before OnUpdate to maintain a fixed step
}

        // runs once per frame before RenderEngine consumes submitted work

        // records UI only after world rendering starts successfully

        // runs after every OnInitialize call, including failed initialization
